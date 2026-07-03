#include "FileRenameUtils.h"

#include <QFileInfo>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

// ── traditionalToSimplified ─────────────────────────────────

QString traditionalToSimplified(const QString &text)
{
    if (text.isEmpty())
        return text;

#ifdef Q_OS_WIN
    const int inputLength = text.length();
    const int requiredLength = LCMapStringEx(
        L"zh-CN",
        LCMAP_SIMPLIFIED_CHINESE,
        reinterpret_cast<LPCWSTR>(text.utf16()),
        inputLength,
        nullptr,
        0,
        nullptr,
        nullptr,
        0);

    if (requiredLength <= 0)
        return text;

    QString converted(requiredLength, Qt::Uninitialized);
    const int convertedLength = LCMapStringEx(
        L"zh-CN",
        LCMAP_SIMPLIFIED_CHINESE,
        reinterpret_cast<LPCWSTR>(text.utf16()),
        inputLength,
        reinterpret_cast<LPWSTR>(converted.data()),
        requiredLength,
        nullptr,
        nullptr,
        0);

    if (convertedLength <= 0)
        return text;

    converted.truncate(convertedLength);
    return converted;
#else
    return text;
#endif
}

// ── getFileExtension ────────────────────────────────────────

QString getFileExtension(const QString &fileName)
{
    int dotIndex = fileName.lastIndexOf('.');
    if (dotIndex > 0 && dotIndex < fileName.length() - 1)
        return fileName.mid(dotIndex);
    return {};
}

bool renameFile(const QString &oldPath, const QString &newPath)
{
    QDir parent(QFileInfo(oldPath).absolutePath());
    return parent.rename(QFileInfo(oldPath).fileName(), QFileInfo(newPath).fileName());
}
