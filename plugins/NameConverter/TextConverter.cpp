#include "TextConverter.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

QString ChineseTextConverter::traditionalToSimplified(const QString &text) const
{
    if (text.isEmpty()) {
        return text;
    }

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

    if (requiredLength <= 0) {
        return text;
    }

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

    if (convertedLength <= 0) {
        return text;
    }

    converted.truncate(convertedLength);
    return converted;
#else
    return text;
#endif
}
