#pragma once
#include <QDir>
#include <QString>

// Only the newly created QTemporaryDir child is owned by a test. The selected
// parent must already exist; no environment variable can authorize its removal.
inline QString operationSandboxTemplate(const QString &prefix) {
    const auto configured = qEnvironmentVariable("REA_OPERATION_TEST_ROOT");
    const auto root =
        configured.isEmpty()
            ? QDir(QString::fromUtf8(EXPLORER_PROJECT_DIR)).filePath(QStringLiteral("test-data"))
            : configured;
    if (!QDir::isAbsolutePath(root) || !QDir(root).exists())
        return {};
    return QDir(root).filePath(prefix + QStringLiteral("-XXXXXX"));
}
