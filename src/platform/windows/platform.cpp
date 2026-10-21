#include "explorer/platform.hpp"
#ifndef STRICT_TYPED_ITEMIDS
#define STRICT_TYPED_ITEMIDS
#endif
#include <windows.h>

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <limits>
#include <memory>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <vector>

namespace explorer {
namespace {
constexpr quint64 listingBudget = 64ULL * 1024 * 1024;
const wchar_t *wide(const QString &value) {
    return reinterpret_cast<const wchar_t *>(value.utf16());
}
QString apiPath(const QString &path) {
    if (path.startsWith(QStringLiteral("\\\\?\\")))
        return path;
    if (path.startsWith(QStringLiteral("\\\\")))
        return QStringLiteral("\\\\?\\UNC\\") + path.mid(2);
    return QStringLiteral("\\\\?\\") + path;
}
qsizetype rootLength(const QString &path) {
    qsizetype serverStart = -1;
    if (path.startsWith(QStringLiteral("\\\\?\\UNC\\"), Qt::CaseInsensitive))
        serverStart = 8;
    else if (path.startsWith(QStringLiteral("\\\\?\\")) && path.size() >= 7 && path[5] == QLatin1Char(':') &&
             path[6] == QLatin1Char('\\'))
        return 7;
    else if (path.startsWith(QStringLiteral("\\\\")))
        serverStart = 2;
    else if (path.size() >= 3 && path[1] == QLatin1Char(':') && path[2] == QLatin1Char('\\'))
        return 3;
    if (serverStart < 0)
        return 0;
    const qsizetype serverEnd = path.indexOf(QLatin1Char('\\'), serverStart);
    if (serverEnd < 0 || serverEnd == serverStart)
        return 0;
    const qsizetype shareEnd = path.indexOf(QLatin1Char('\\'), serverEnd + 1);
    if (serverEnd + 1 == path.size() || shareEnd == serverEnd + 1)
        return 0;
    return shareEnd < 0 ? path.size() : shareEnd + 1;
}
Error winError(const QString &operation, const QString &input, DWORD code) {
    ErrorCode kind = ErrorCode::Io;
    if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND || code == ERROR_INVALID_DRIVE)
        kind = ErrorCode::NotFound;
    else if (code == ERROR_ACCESS_DENIED || code == ERROR_PRIVILEGE_NOT_HELD ||
             code == ERROR_NETWORK_ACCESS_DENIED)
        kind = ErrorCode::PermissionDenied;
    else if (code == ERROR_INVALID_NAME || code == ERROR_BAD_PATHNAME || code == ERROR_DIRECTORY)
        kind = ErrorCode::InvalidPath;
    else if (code == ERROR_FILENAME_EXCED_RANGE || code == ERROR_NOT_SUPPORTED)
        kind = ErrorCode::Unsupported;
    wchar_t *buffer = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<wchar_t *>(&buffer), 0, nullptr);
    QString message =
        buffer ? QString::fromWCharArray(buffer).trimmed() : QStringLiteral("Windows error %1").arg(code);
    if (buffer)
        LocalFree(buffer);
    return {kind, operation, input, message, static_cast<qint64>(code), {}};
}
Error comError(const QString &operation, const QString &input, HRESULT hr) {
    Error result = winError(
        operation, input, HRESULT_FACILITY(hr) == FACILITY_WIN32 ? HRESULT_CODE(hr) : static_cast<DWORD>(hr));
    result.nativeCode = static_cast<qint64>(hr);
    return result;
}
struct Apartment {
    HRESULT status = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ~Apartment() {
        if (SUCCEEDED(status))
            CoUninitialize();
    }
};
template <class T> struct ComDelete {
    void operator()(T *pointer) const {
        if (pointer)
            pointer->Release();
    }
};
template <class T> using ComPtr = std::unique_ptr<T, ComDelete<T>>;
struct TaskDelete {
    void operator()(void *pointer) const {
        CoTaskMemFree(pointer);
    }
};
using AbsolutePidl = std::unique_ptr<ITEMIDLIST_ABSOLUTE, TaskDelete>;
using ChildPidl = std::unique_ptr<ITEMID_CHILD, TaskDelete>;
struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~Handle() {
        if (value != INVALID_HANDLE_VALUE)
            CloseHandle(value);
    }
};
QString identity(const QString &path) {
    const QString native = apiPath(path);
    Handle handle{CreateFileW(wide(native), FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                              FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr)};
    if (handle.value == INVALID_HANDLE_VALUE)
        return {};
    FILE_ID_INFO info{};
    if (!GetFileInformationByHandleEx(handle.value, FileIdInfo, &info, sizeof(info)))
        return {};
    const QByteArray id(reinterpret_cast<const char *>(info.FileId.Identifier),
                        sizeof(info.FileId.Identifier));
    return QString::number(info.VolumeSerialNumber, 16) + QLatin1Char(':') + QString::fromLatin1(id.toHex());
}
std::optional<qint64> modified(const FILETIME &time) {
    const quint64 ticks = (static_cast<quint64>(time.dwHighDateTime) << 32) | time.dwLowDateTime;
    if (ticks == 0)
        return {};
    constexpr quint64 epoch = 116444736000000000ULL;
    if (ticks >= epoch)
        return static_cast<qint64>((ticks - epoch) / 10000);
    return -static_cast<qint64>((epoch - ticks) / 10000);
}
void metadata(Entry &entry) {
    WIN32_FIND_DATAW data{};
    const QString native = apiPath(entry.path);
    HANDLE search = FindFirstFileW(wide(native), &data);
    if (search != INVALID_HANDLE_VALUE) {
        FindClose(search);
        entry.directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entry.link = (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        if (!entry.directory)
            entry.sizeBytes = (static_cast<quint64>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
        entry.modifiedMs = modified(data.ftLastWriteTime);
    }
    entry.identity = identity(entry.path);
    SHFILEINFOW info{};
    if (SHGetFileInfoW(wide(entry.path), 0, &info, sizeof(info), SHGFI_TYPENAME))
        entry.typeName = QString::fromWCharArray(info.szTypeName);
    if (entry.typeName.isEmpty())
        entry.typeName = entry.directory ? QStringLiteral("Folder") : QStringLiteral("File");
}
quint64 estimated(const Entry &entry) {
    return sizeof(Entry) +
           2ULL * static_cast<quint64>(entry.key.size() + entry.path.size() + entry.name.size() +
                                       entry.typeName.size() + entry.identity.size());
}
Error withPartial(Error error, const QVector<Entry> &entries) {
    error.partialPaths.reserve(entries.size());
    for (const auto &entry : entries)
        error.partialPaths.push_back(entry.path);
    return error;
}
Error cancelled(const Location &location, const QVector<Entry> &entries) {
    return withPartial({ErrorCode::Cancelled,
                        QStringLiteral("list"),
                        location.callerInput,
                        QStringLiteral("Directory loading cancelled"),
                        0,
                        {}},
                       entries);
}

class WindowsPlatform final : public IPlatform {
  public:
    Result<Location> parseLocation(const QString &input) const override {
        if (input.isEmpty() || input.contains(QChar(u'\0')) || input.startsWith(QStringLiteral("\\\\.\\")))
            return Error{ErrorCode::InvalidPath,
                         QStringLiteral("parse"),
                         input,
                         QStringLiteral("Enter a filesystem directory path"),
                         0,
                         {}};
        const QString supplied = QDir::toNativeSeparators(input);
        DWORD needed = GetFullPathNameW(wide(supplied), 0, nullptr, nullptr);
        if (!needed)
            return winError(QStringLiteral("parse"), input, GetLastError());
        if (needed > 32768)
            return winError(QStringLiteral("parse"), input, ERROR_FILENAME_EXCED_RANGE);
        std::vector<wchar_t> buffer(static_cast<size_t>(needed));
        const DWORD length = GetFullPathNameW(wide(supplied), needed, buffer.data(), nullptr);
        if (!length)
            return winError(QStringLiteral("parse"), input, GetLastError());
        if (length >= needed)
            return Error{ErrorCode::Changed,
                         QStringLiteral("parse"),
                         input,
                         QStringLiteral("Path resolution changed; retry"),
                         0,
                         {}};
        const QString path = QString::fromWCharArray(buffer.data(), static_cast<int>(length));
        const QString native = apiPath(path);
        const DWORD attributes = GetFileAttributesW(wide(native));
        if (attributes == INVALID_FILE_ATTRIBUTES)
            return winError(QStringLiteral("parse"), input, GetLastError());
        if (!(attributes & FILE_ATTRIBUTE_DIRECTORY))
            return winError(QStringLiteral("parse"), input, ERROR_DIRECTORY);
        QString label = QFileInfo(path).fileName();
        if (label.isEmpty())
            label = path;
        return Location{Location::Kind::Directory, input, path, path, label};
    }

    Result<std::optional<Location>> parent(const Location &location) const override {
        if (location.kind == Location::Kind::Drives)
            return std::optional<Location>{};
        QString path = QDir::toNativeSeparators(location.path);
        const qsizetype root = rootLength(path);
        if (!root)
            return Error{ErrorCode::InvalidPath,
                         QStringLiteral("parent"),
                         location.path,
                         QStringLiteral("Location is not an absolute filesystem path"),
                         0,
                         {}};
        while (path.size() > root && path.endsWith(QLatin1Char('\\')))
            path.chop(1);
        if (path.size() <= root)
            return std::optional<Location>{Location{}};
        const qsizetype separator = path.lastIndexOf(QLatin1Char('\\'));
        const QString parentPath = path.left(separator < root ? root : separator);
        QString label = QFileInfo(parentPath).fileName();
        if (label.isEmpty())
            label = parentPath;
        // No filesystem query: the asynchronous list request validates this lexical parent.
        return std::optional<Location>{
            Location{Location::Kind::Directory, parentPath, parentPath, parentPath, label}};
    }

    Result<Listing> list(const Location &location, std::stop_token stop,
                         ProgressSink progress) const override {
        QVector<Entry> entries;
        quint64 bytes = 0;
        const auto append = [&](Entry entry) -> std::optional<Error> {
            const quint64 addition = estimated(entry);
            if (addition > listingBudget - bytes)
                return withPartial({ErrorCode::ResourceLimit,
                                    QStringLiteral("list"),
                                    location.callerInput,
                                    QStringLiteral("Directory listing exceeds the 64 MiB estimated budget"),
                                    0,
                                    {}},
                                   entries);
            bytes += addition;
            entries.push_back(std::move(entry));
            if (progress)
                progress({static_cast<quint64>(entries.size()), QStringLiteral("Reading directory")});
            return {};
        };
        if (stop.stop_requested())
            return cancelled(location, entries);
        Apartment apartment;
        if (FAILED(apartment.status))
            return comError(QStringLiteral("COM initialization"), location.path, apartment.status);
        if (location.kind == Location::Kind::Drives) {
            DWORD needed = GetLogicalDriveStringsW(0, nullptr);
            if (!needed)
                return winError(QStringLiteral("list drives"), location.callerInput, GetLastError());
            std::vector<wchar_t> buffer(static_cast<size_t>(needed) + 1);
            DWORD length = GetLogicalDriveStringsW(static_cast<DWORD>(buffer.size()), buffer.data());
            if (!length)
                return winError(QStringLiteral("list drives"), location.callerInput, GetLastError());
            if (length >= buffer.size())
                return Error{ErrorCode::Changed,
                             QStringLiteral("list drives"),
                             {},
                             QStringLiteral("Drive set changed; retry"),
                             0,
                             {}};
            for (const wchar_t *drive = buffer.data(); *drive; drive += wcslen(drive) + 1) {
                if (stop.stop_requested())
                    return cancelled(location, entries);
                Entry entry;
                entry.path = QString::fromWCharArray(drive);
                entry.key = entry.path;
                entry.name = entry.path;
                entry.directory = true;
                entry.typeName = QStringLiteral("Local drive");
                entry.identity = identity(entry.path);
                if (auto error = append(std::move(entry)))
                    return *error;
            }
            return Listing{location, std::move(entries), true};
        }
        // Shell implementations may translate permission failures into an empty enumerator.
        // Require actual list-directory access before interpreting a Shell empty result.
        const QString native = apiPath(location.path);
        Handle access{CreateFileW(wide(native), FILE_LIST_DIRECTORY,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                  OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr)};
        if (access.value == INVALID_HANDLE_VALUE)
            return winError(QStringLiteral("list"), location.callerInput, GetLastError());
        PIDLIST_ABSOLUTE parsed = nullptr;
        HRESULT hr = SHParseDisplayName(wide(location.path), nullptr, &parsed, 0, nullptr);
        AbsolutePidl folderId(parsed);
        if (FAILED(hr))
            return comError(QStringLiteral("parse Shell folder"), location.callerInput, hr);
        if (!folderId)
            return comError(QStringLiteral("parse Shell folder"), location.callerInput, E_UNEXPECTED);
        IShellFolder *desktopRaw = nullptr;
        hr = SHGetDesktopFolder(&desktopRaw);
        ComPtr<IShellFolder> desktop(desktopRaw);
        if (FAILED(hr))
            return comError(QStringLiteral("desktop folder"), location.callerInput, hr);
        if (!desktop)
            return comError(QStringLiteral("desktop folder"), location.callerInput, E_UNEXPECTED);
        IShellFolder *folderRaw = nullptr;
        hr = desktop->BindToObject(folderId.get(), nullptr, IID_IShellFolder,
                                   reinterpret_cast<void **>(&folderRaw));
        ComPtr<IShellFolder> folder(folderRaw);
        if (FAILED(hr))
            return comError(QStringLiteral("bind folder"), location.callerInput, hr);
        if (!folder)
            return comError(QStringLiteral("bind folder"), location.callerInput, E_UNEXPECTED);
        IEnumIDList *enumRaw = nullptr;
        hr = folder->EnumObjects(
            nullptr, static_cast<SHCONTF>(SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN),
            &enumRaw);
        ComPtr<IEnumIDList> enumerator(enumRaw);
        if (FAILED(hr))
            return comError(QStringLiteral("enumerate folder"), location.callerInput, hr);
        if (!enumerator)
            return Listing{location, {}, true};
        while (true) {
            if (stop.stop_requested())
                return cancelled(location, entries);
            PITEMID_CHILD childRaw = nullptr;
            ULONG fetched = 0;
            hr = enumerator->Next(1, &childRaw, &fetched);
            ChildPidl child(childRaw);
            if (FAILED(hr))
                return withPartial(comError(QStringLiteral("next folder item"), location.callerInput, hr),
                                   entries);
            if (hr == S_FALSE || fetched == 0)
                break;
            if (!child || fetched != 1)
                return withPartial({ErrorCode::Io,
                                    QStringLiteral("next folder item"),
                                    location.path,
                                    QStringLiteral("Invalid Shell enumerator result"),
                                    0,
                                    {}},
                                   entries);
            PCUITEMID_CHILD childArray[] = {child.get()};
            SFGAOF attributes = SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_LINK;
            hr = folder->GetAttributesOf(1, childArray, &attributes);
            if (FAILED(hr))
                return withPartial(comError(QStringLiteral("item attributes"), location.path, hr), entries);
            if (!(attributes & SFGAO_FILESYSTEM))
                continue;
            AbsolutePidl absolute(ILCombine(folderId.get(), child.get()));
            if (!absolute)
                return withPartial({ErrorCode::ResourceLimit,
                                    QStringLiteral("item PIDL"),
                                    location.path,
                                    QStringLiteral("Cannot allocate Shell item identity"),
                                    0,
                                    {}},
                                   entries);
            wchar_t *pathRaw = nullptr;
            hr = SHGetNameFromIDList(absolute.get(), SIGDN_FILESYSPATH, &pathRaw);
            std::unique_ptr<wchar_t, TaskDelete> pathMemory(pathRaw);
            if (FAILED(hr) || !pathRaw)
                return withPartial(
                    comError(QStringLiteral("item filesystem path"), location.path, FAILED(hr) ? hr : E_FAIL),
                    entries);
            Entry entry;
            entry.path = QString::fromWCharArray(pathRaw);
            entry.key = entry.path;
            entry.directory = (attributes & SFGAO_FOLDER) != 0;
            entry.link = (attributes & SFGAO_LINK) != 0;
            STRRET display{};
            hr = folder->GetDisplayNameOf(child.get(), SHGDN_NORMAL, &display);
            wchar_t *nameRaw = nullptr;
            if (SUCCEEDED(hr))
                hr = StrRetToStrW(&display, child.get(), &nameRaw);
            std::unique_ptr<wchar_t, TaskDelete> nameMemory(nameRaw);
            if (FAILED(hr) || !nameRaw)
                return withPartial(
                    comError(QStringLiteral("item display name"), entry.path, FAILED(hr) ? hr : E_FAIL),
                    entries);
            // Actual filename remains the search/name key; Shell display text may hide extensions.
            entry.name = QFileInfo(entry.path).fileName();
            if (entry.name.isEmpty())
                entry.name = QString::fromWCharArray(nameRaw);
            metadata(entry);
            if (auto error = append(std::move(entry)))
                return *error;
        }
        if (stop.stop_requested())
            return cancelled(location, entries);
        return Listing{location, std::move(entries), true};
    }

    int compareNames(const QString &a, const QString &b) const override {
        return StrCmpLogicalW(wide(a), wide(b));
    }

    Result<Unit> open(const Entry &entry) const override {
        const QString native = apiPath(entry.path);
        if (GetFileAttributesW(wide(native)) == INVALID_FILE_ATTRIBUTES)
            return winError(QStringLiteral("open"), entry.path, GetLastError());
        const QString current = identity(entry.path);
        if (entry.identity.isEmpty() || current.isEmpty() || current != entry.identity)
            return Error{ErrorCode::Changed,
                         QStringLiteral("open"),
                         entry.path,
                         QStringLiteral("File identity is unknown or changed; refresh before opening"),
                         0,
                         {}};
        Apartment apartment;
        if (FAILED(apartment.status))
            return comError(QStringLiteral("COM initialization"), entry.path, apartment.status);
        PIDLIST_ABSOLUTE raw = nullptr;
        HRESULT hr = SHParseDisplayName(wide(entry.path), nullptr, &raw, 0, nullptr);
        AbsolutePidl pidl(raw);
        if (FAILED(hr))
            return comError(QStringLiteral("parse Shell item"), entry.path, hr);
        if (!pidl)
            return comError(QStringLiteral("parse Shell item"), entry.path, E_UNEXPECTED);
        const QString directory = QDir::toNativeSeparators(QFileInfo(entry.path).absolutePath());
        SHELLEXECUTEINFOW request{};
        request.cbSize = sizeof(request);
        // INVOKEIDLIST is SDK 0xC (REA OPEN). NOASYNC keeps this worker apartment
        // alive until Shell delegation is complete; it does not wait for the opened app.
        request.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_NOASYNC;
        request.lpVerb = L"";
        request.lpIDList = pidl.get();
        request.lpDirectory = wide(directory);
        request.nShow = SW_SHOWNORMAL;
        if (!ShellExecuteExW(&request))
            return winError(QStringLiteral("open"), entry.path, GetLastError());
        return Unit{};
    }

    Capabilities capabilities() const override {
        Capabilities result;
        result.open = true;
        return result;
    }
};
} // namespace
std::shared_ptr<IPlatform> makePlatform() {
    return std::make_shared<WindowsPlatform>();
}
} // namespace explorer
