#include "file_operations.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QUuid>
#include <filesystem>
#include <limits>

namespace explorer::platform_detail {
namespace {
namespace fs = std::filesystem;
constexpr qsizetype maxNodes = 100000;
constexpr quint64 maxManifestBytes = 64ULL * 1024 * 1024;
Error problem(ErrorCode code, const QString &path, const QString &message) {
    return {code, QStringLiteral("file operation"), path, message, 0, {}};
}
Error cancelled(const QString &path) {
    return problem(ErrorCode::Cancelled, path, QStringLiteral("Operation cancelled"));
}
QString clean(const QString &path) {
    return QDir::cleanPath(QDir::fromNativeSeparators(path));
}
fs::path nativePath(const QString &path) {
#ifdef Q_OS_WIN
    return fs::path(path.toStdWString());
#else
    return fs::path(QFile::encodeName(path).constData());
#endif
}
QString displayPath(const fs::path &path) {
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.native());
#else
    return QFile::decodeName(path.native().c_str());
#endif
}
Result<QStringList> children(const QString &path) {
    std::error_code error;
    fs::directory_iterator iterator(nativePath(path), fs::directory_options::none, error), end;
    if (error)
        return Error{error == std::errc::permission_denied ? ErrorCode::PermissionDenied : ErrorCode::Io,
                     QStringLiteral("enumerate operation directory"),
                     path,
                     QString::fromStdString(error.message()),
                     error.value(),
                     {}};
    QStringList result;
    while (iterator != end) {
        if (result.size() >= maxNodes)
            return problem(ErrorCode::ResourceLimit, path,
                           QStringLiteral("Operation directory exceeds 100000 entries"));
        const QString child = displayPath(iterator->path());
        if (nativePath(child) != iterator->path())
            return problem(ErrorCode::Unsupported, path,
                           QStringLiteral("Filename cannot round-trip through the filesystem codec"));
        result.push_back(child);
        iterator.increment(error);
        if (error)
            return Error{error == std::errc::permission_denied ? ErrorCode::PermissionDenied : ErrorCode::Io,
                         QStringLiteral("enumerate operation directory"),
                         path,
                         QString::fromStdString(error.message()),
                         error.value(),
                         result};
    }
    return result;
}
bool sameNode(const Node &a, const Node &b, bool contents) {
    return !a.identity.isEmpty() && a.identity == b.identity && a.directory == b.directory && !b.link &&
           (!contents || a.directory || (a.size == b.size && a.modified == b.modified));
}
bool safeLeaf(const QString &name, bool windows) {
    if (name.isEmpty() || name == QStringLiteral(".") || name == QStringLiteral("..") ||
        name.contains(QChar(u'\0')) || name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\')))
        return false;
    if (!windows)
        return true;
    if (name.endsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char(' ')))
        return false;
    for (const auto character : name)
        if (character.unicode() < 32 || QStringLiteral("<>:\"|?*").contains(character))
            return false;
    const QString base = name.section(QLatin1Char('.'), 0, 0).toUpper();
    if (base == QStringLiteral("CON") || base == QStringLiteral("PRN") || base == QStringLiteral("AUX") ||
        base == QStringLiteral("NUL"))
        return false;
    if (base.size() == 4 &&
        (base.startsWith(QStringLiteral("COM")) || base.startsWith(QStringLiteral("LPT"))) &&
        base[3] >= QLatin1Char('1') && base[3] <= QLatin1Char('9'))
        return false;
    return true;
}
} // namespace

Result<Node> FileOperations::safeInspect(const QString &path) const {
    if (!QDir::isAbsolutePath(path))
        return problem(ErrorCode::InvalidPath, path,
                       QStringLiteral("An absolute filesystem path is required"));
    // Reject links/reparse points in every ancestor, not only the selected leaf.
    QString current = clean(path);
    for (int depth = 0; depth < 256; ++depth) {
        auto found = native_.inspect(current);
        if (const auto *error = failure(found))
            return *error;
        const auto &node = std::get<Node>(found);
        if (node.link)
            return problem(ErrorCode::Unsupported, current,
                           QStringLiteral("File operations do not follow symbolic links or reparse points"));
        if (node.identity.isEmpty())
            return problem(ErrorCode::Changed, current,
                           QStringLiteral("Filesystem object identity is unknown"));
        const QString parent = clean(QFileInfo(current).absolutePath());
        if (parent == current)
            return native_.inspect(path);
        current = parent;
    }
    return problem(ErrorCode::ResourceLimit, path, QStringLiteral("Path ancestor depth exceeds 256"));
}

Result<FileOperations::Tree> FileOperations::snapshot(const QString &path, std::stop_token stop) const {
    Tree result;
    QVector<QPair<QString, int>> pending{{path, 0}};
    quint64 bytes = 0;
    while (!pending.isEmpty()) {
        if (stop.stop_requested())
            return cancelled(path);
        auto [current, depth] = pending.takeLast();
        if (depth > 256 || result.size() >= maxNodes)
            return problem(ErrorCode::ResourceLimit, current,
                           QStringLiteral("Operation tree exceeds depth or entry bounds"));
        auto found = safeInspect(current);
        if (const auto *error = failure(found))
            return *error;
        Node node = std::get<Node>(std::move(found));
        const quint64 addition =
            sizeof(OwnedNode) + 2ULL * static_cast<quint64>(current.size() + node.identity.size());
        if (addition > maxManifestBytes - bytes)
            return problem(ErrorCode::ResourceLimit, path,
                           QStringLiteral("Operation manifest exceeds 64 MiB"));
        bytes += addition;
        result.push_back({current, node});
        if (node.directory) {
            auto listed = children(current);
            if (const auto *error = failure(listed))
                return *error;
            const auto &paths = std::get<QStringList>(listed);
            if (paths.size() > maxNodes - result.size() - pending.size())
                return problem(ErrorCode::ResourceLimit, path,
                               QStringLiteral("Operation tree exceeds 100000 entries"));
            for (const auto &child : paths)
                pending.push_back({child, depth + 1});
        }
    }
    return result;
}

Result<Unit> FileOperations::verify(const Tree &tree, bool contents) const {
    QSet<QString> expected;
    for (const auto &item : tree)
        expected.insert(clean(item.path));
    for (const auto &item : tree) {
        auto current = safeInspect(item.path);
        if (const auto *error = failure(current))
            return *error;
        if (!sameNode(item.node, std::get<Node>(current), contents))
            return problem(ErrorCode::Changed, item.path,
                           QStringLiteral("Filesystem object changed since the operation snapshot"));
        if (item.node.directory) {
            auto listed = children(item.path);
            if (const auto *error = failure(listed))
                return *error;
            for (const auto &child : std::get<QStringList>(listed))
                if (!expected.contains(clean(child)))
                    return problem(ErrorCode::Changed, child,
                                   QStringLiteral("Unexpected child appeared during the operation"));
        }
    }
    return Unit{};
}

Result<Unit> FileOperations::removeTree(Tree &tree, std::stop_token stop, bool contents) const {
    // Remove only the manifest's exact identities; no remove_all or name-only ownership.
    // A previously owned node may already be absent after a partial cleanup or
    // an external deletion. Reconcile only authoritative NotFound, never a
    // lookup failure, permission denial, or a different identity.
    for (qsizetype index = tree.size(); index > 0; --index) {
        auto current = native_.inspect(tree[index - 1].path);
        if (const auto *error = failure(current)) {
            if (error->code == ErrorCode::NotFound)
                tree.removeAt(index - 1);
            else
                return *error;
        }
    }
    auto verified = verify(tree, contents);
    if (const auto *error = failure(verified))
        return *error;
    while (!tree.isEmpty()) {
        if (stop.stop_requested())
            return cancelled(tree.last().path);
        auto result = native_.remove(tree.last().path, tree.last().node, contents);
        if (const auto *error = failure(result)) {
            if (error->code != ErrorCode::NotFound)
                return *error;
        }
        tree.removeLast();
    }
    return Unit{};
}

Result<Unit> FileOperations::retryCleanup(const QString &path) const {
    std::lock_guard lock(mutex_);
    auto found = cleanup_.find(path);
    if (found == cleanup_.end())
        return problem(ErrorCode::Unsupported, path,
                       QStringLiteral("This adapter does not own retained cleanup at this path"));
    auto result = removeTree(found.value());
    if (!failure(result))
        cleanup_.erase(found);
    return result;
}

OperationReport FileOperations::execute(const OperationRequest &request, std::stop_token stop,
                                        OperationProgressSink sink) const {
    std::lock_guard lock(mutex_);
    OperationReport report;
    report.kind = request.kind;
    const auto rejectRequest = [&](Error error) {
        report.items.push_back({{}, {}, ItemStatus::Failed, std::move(error), {}});
        return report;
    };
    if (request.kind != OperationKind::CreateDirectory && request.sources.isEmpty())
        return rejectRequest(
            problem(ErrorCode::InvalidPath, {}, QStringLiteral("At least one source entry is required")));
    if (request.sources.size() > 20000)
        return rejectRequest(
            problem(ErrorCode::ResourceLimit, {},
                    QStringLiteral("An operation request may contain at most 20000 source entries")));
    quint64 requestBytes = 0;
    for (const auto &source : request.sources) {
        const quint64 addition =
            sizeof(Entry) +
            2ULL * static_cast<quint64>(source.key.size() + source.path.size() + source.name.size() +
                                        source.typeName.size() + source.identity.size());
        if (addition > maxManifestBytes - requestBytes)
            return rejectRequest(problem(
                ErrorCode::ResourceLimit, {},
                QStringLiteral("Operation source metadata exceeds the 64 MiB estimated request budget")));
        requestBytes += addition;
    }
    OperationProgress progress;
    const bool mkdir = request.kind == OperationKind::CreateDirectory;
    const qsizetype count = mkdir ? 1 : request.sources.size();
    progress.totalItems = static_cast<quint64>(count);
    auto emitProgress = [&](const QString &path, const QString &phase) {
        progress.currentPath = path;
        progress.phase = phase;
        if (sink)
            sink(progress);
    };
    auto destinationValid = [&]() -> Result<Node> {
        if (request.destination.kind != Location::Kind::Directory || request.destination.identity.isEmpty())
            return problem(ErrorCode::Changed, request.destination.path,
                           QStringLiteral("A current destination directory identity is required"));
        auto node = safeInspect(request.destination.path);
        if (const auto *error = failure(node))
            return *error;
        if (!std::get<Node>(node).directory || std::get<Node>(node).identity != request.destination.identity)
            return problem(ErrorCode::Changed, request.destination.path,
                           QStringLiteral("Destination directory identity changed"));
        return node;
    };
    for (qsizetype index = 0; index < count; ++index) {
        ItemOutcome outcome;
        const Entry source = mkdir ? Entry{} : request.sources[index];
        outcome.sourcePath = source.path;
        auto failItem = [&](Error error) {
            outcome.status = error.code == ErrorCode::Cancelled ? ItemStatus::Cancelled : ItemStatus::Failed;
            outcome.error = std::move(error);
        };
        if (stop.stop_requested()) {
            failItem(cancelled(source.path));
            report.cancelled = true;
        } else if (request.kind == OperationKind::Rename && count != 1) {
            failItem(problem(ErrorCode::InvalidPath, source.path,
                             QStringLiteral("Rename requires exactly one source")));
        } else {
            QString stage;
            Tree owned;
            auto run = [&]() -> Result<Unit> {
                if (mkdir && !request.sources.isEmpty())
                    return problem(ErrorCode::InvalidPath, {},
                                   QStringLiteral("Create-directory does not accept source entries"));
                Tree sourceTree;
                if (!mkdir) {
                    if (source.identity.isEmpty())
                        return problem(ErrorCode::Changed, source.path,
                                       QStringLiteral("A known source identity is required"));
                    auto tree = snapshot(source.path, stop);
                    if (const auto *error = failure(tree))
                        return *error;
                    sourceTree = std::get<Tree>(std::move(tree));
                    if (sourceTree.isEmpty() || sourceTree.first().node.identity != source.identity)
                        return problem(ErrorCode::Changed, source.path,
                                       QStringLiteral("Source identity changed"));
                    if (source.link)
                        return problem(ErrorCode::Unsupported, source.path,
                                       QStringLiteral("Link operations are unsupported"));
                    if (count == 1 && request.kind == OperationKind::Copy) {
                        quint64 total = 0;
                        bool known = true;
                        for (const auto &node : sourceTree)
                            if (!node.node.directory) {
                                if (node.node.size > std::numeric_limits<quint64>::max() - total) {
                                    known = false;
                                    break;
                                }
                                total += node.node.size;
                            }
                        if (known)
                            progress.totalBytes = total;
                    }
                }
                if (request.kind == OperationKind::Recycle) {
                    if (stop.stop_requested())
                        return cancelled(source.path);
                    auto verified = verify(sourceTree, true);
                    if (const auto *error = failure(verified))
                        return *error;
                    emitProgress(source.path, QStringLiteral("Moving to recycle/trash"));
                    if (stop.stop_requested())
                        return cancelled(source.path);
                    return native_.recycle(source.path, sourceTree.first().node);
                }
                auto destination = destinationValid();
                if (const auto *error = failure(destination))
                    return *error;
                const QString leaf = mkdir || request.kind == OperationKind::Rename
                                         ? request.name
                                         : QFileInfo(source.path).fileName();
                if (!safeLeaf(leaf, native_.windowsPaths))
                    return problem(ErrorCode::InvalidPath, leaf,
                                   QStringLiteral("A safe single filename is required"));
                const QString directory = clean(request.destination.path);
                const QString original = clean(source.path);
                const Qt::CaseSensitivity casePolicy =
                    native_.windowsPaths ? Qt::CaseInsensitive : Qt::CaseSensitive;
                if (!mkdir && sourceTree.first().node.directory &&
                    (directory.compare(original, casePolicy) == 0 ||
                     directory.startsWith(original + QLatin1Char('/'), casePolicy)))
                    return problem(ErrorCode::InvalidPath, directory,
                                   QStringLiteral("Cannot copy or move a directory into its own subtree"));
                auto chooseDestination = [&]() -> Result<QString> {
                    for (int suffix = 0; suffix <= 10000; ++suffix) {
                        const QString candidateName =
                            suffix ? leaf + QStringLiteral(" (%1)").arg(suffix) : leaf;
                        const QString candidate = QDir(directory).filePath(candidateName);
                        auto exists = native_.inspect(candidate);
                        if (const auto *error = failure(exists)) {
                            if (error->code == ErrorCode::NotFound)
                                return candidate;
                            return *error;
                        }
                        if (request.conflict == ConflictPolicy::Fail)
                            return problem(
                                ErrorCode::Conflict, candidate,
                                QStringLiteral("Destination already exists; no file was overwritten"));
                    }
                    return problem(ErrorCode::ResourceLimit, directory,
                                   QStringLiteral("Keep-both name attempts exhausted"));
                };
                auto selected = chooseDestination();
                if (const auto *error = failure(selected))
                    return *error;
                outcome.destinationPath = std::get<QString>(selected);
                if (request.kind == OperationKind::Move || request.kind == OperationKind::Rename) {
                    auto checked = verify(sourceTree, true);
                    if (const auto *error = failure(checked))
                        return *error;
                    auto dest = destinationValid();
                    if (const auto *error = failure(dest))
                        return *error;
                    if (stop.stop_requested())
                        return cancelled(source.path);
                    emitProgress(source.path, QStringLiteral("Moving without replacement"));
                    if (stop.stop_requested())
                        return cancelled(source.path);
                    auto moved = native_.moveNoReplace(source.path, sourceTree.first().node,
                                                       outcome.destinationPath, std::get<Node>(dest));
                    if (const auto *error = failure(moved)) {
                        if (!error->partialPaths.isEmpty()) {
                            outcome.status = ItemStatus::Partial;
                            outcome.retainedPaths += error->partialPaths;
                        }
                        return *error;
                    }
                    if (std::get<bool>(moved))
                        return Unit{};
                    if (request.kind == OperationKind::Rename)
                        return problem(ErrorCode::Unsupported, source.path,
                                       QStringLiteral("Rename across filesystems is unsupported; use Move"));
                }
                // Stage in the destination directory. Every created node is owned by exact identity.
                stage = QDir(directory).filePath(
                    QStringLiteral(".rea-stage-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
                auto created = native_.createExclusive(stage, mkdir || sourceTree.first().node.directory);
                if (const auto *error = failure(created)) {
                    if (!error->partialPaths.isEmpty()) {
                        owned.push_back({stage, {}});
                        outcome.retainedPaths += error->partialPaths;
                    }
                    return *error;
                }
                owned.push_back({stage, std::get<Node>(created)});
                if (!mkdir) {
                    for (qsizetype nodeIndex = 0; nodeIndex < sourceTree.size(); ++nodeIndex) {
                        if (stop.stop_requested())
                            return cancelled(source.path);
                        const auto &node = sourceTree[nodeIndex];
                        const QString relative =
                            nodeIndex == 0 ? QString{} : QDir(source.path).relativeFilePath(node.path);
                        const QString target = relative.isEmpty() ? stage : QDir(stage).filePath(relative);
                        if (nodeIndex > 0) {
                            auto child = native_.createExclusive(target, node.node.directory);
                            if (const auto *error = failure(child)) {
                                if (!error->partialPaths.isEmpty())
                                    owned.push_back({target, {}});
                                return *error;
                            }
                            owned.push_back({target, std::get<Node>(child)});
                        }
                        if (!node.node.directory) {
                            emitProgress(node.path, QStringLiteral("Copying bytes"));
                            auto copied = native_.copyContents(
                                node.path, node.node, target, owned.last().node, stop, [&](quint64 bytes) {
                                    if (bytes <=
                                        std::numeric_limits<quint64>::max() - progress.completedBytes)
                                        progress.completedBytes += bytes;
                                    emitProgress(node.path, QStringLiteral("Copying bytes"));
                                });
                            if (const auto *error = failure(copied))
                                return *error;
                            auto finished = native_.inspect(target);
                            if (const auto *error = failure(finished))
                                return *error;
                            if (std::get<Node>(finished).identity != owned.last().node.identity ||
                                std::get<Node>(finished).link)
                                return problem(ErrorCode::Changed, target,
                                               QStringLiteral("Staging identity changed after copying"));
                            owned.last().node = std::get<Node>(finished);
                        }
                    }
                    auto checked = verify(sourceTree, true);
                    if (const auto *error = failure(checked))
                        return *error;
                }
                auto dest = destinationValid();
                if (const auto *error = failure(dest))
                    return *error;
                auto ownedValid = verify(owned, false);
                if (const auto *error = failure(ownedValid))
                    return *error;
                if (stop.stop_requested())
                    return cancelled(source.path);
                emitProgress(stage, QStringLiteral("Committing without replacement"));
                if (stop.stop_requested())
                    return cancelled(source.path);
                auto committed = native_.moveNoReplace(stage, owned.first().node, outcome.destinationPath,
                                                       std::get<Node>(dest));
                if (const auto *error = failure(committed)) {
                    if (!error->partialPaths.isEmpty()) {
                        outcome.status = ItemStatus::Partial;
                        outcome.retainedPaths += error->partialPaths;
                    }
                    return *error;
                }
                if (!std::get<bool>(committed))
                    return problem(ErrorCode::Io, stage,
                                   QStringLiteral("Staging commit unexpectedly crossed filesystems"));
                stage.clear();
                owned.clear();
                if (request.kind == OperationKind::Move) {
                    // Cross-volume move is deliberately reported as partial if source cleanup cannot finish.
                    auto checked = verify(sourceTree, true);
                    Result<Unit> removed = failure(checked) ? checked : removeTree(sourceTree, stop, true);
                    if (const auto *error = failure(removed)) {
                        outcome.status = ItemStatus::Partial;
                        outcome.retainedPaths << source.path << outcome.destinationPath;
                        return *error;
                    }
                }
                return Unit{};
            };
            auto result = run();
            if (const auto *error = failure(result)) {
                const bool partial = outcome.status == ItemStatus::Partial;
                failItem(*error);
                if (partial)
                    outcome.status = ItemStatus::Partial;
                if (error->code == ErrorCode::Cancelled)
                    report.cancelled = true;
                if (!stage.isEmpty() && !owned.isEmpty()) {
                    // Final cleanup ignores a requested stop but remains identity-checked and retryable.
                    auto cleaned = removeTree(owned);
                    if (const auto *cleanupError = failure(cleaned)) {
                        cleanup_.insert(stage, owned);
                        report.cleanupPending.push_back(stage);
                        if (!outcome.retainedPaths.contains(stage))
                            outcome.retainedPaths.push_back(stage);
                        outcome.error->message +=
                            QStringLiteral("; cleanup failed: %1 (error kind %2, native %3); retained %4")
                                .arg(cleanupError->message)
                                .arg(static_cast<int>(cleanupError->code))
                                .arg(cleanupError->nativeCode)
                                .arg(stage);
                        outcome.error->partialPaths += cleanupError->partialPaths;
                        outcome.error->partialPaths.push_back(stage);
                        outcome.status = ItemStatus::Partial;
                    }
                }
            } else
                outcome.status = ItemStatus::Succeeded;
        }
        report.items.push_back(std::move(outcome));
        ++progress.completedItems;
        emitProgress(source.path, QStringLiteral("Item settled"));
    }
    for (auto item = cleanup_.cbegin(); item != cleanup_.cend(); ++item)
        if (!report.cleanupPending.contains(item.key()))
            report.cleanupPending.push_back(item.key());
    return report;
}
} // namespace explorer::platform_detail
