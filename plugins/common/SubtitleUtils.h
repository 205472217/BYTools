#pragma once

#include <QString>

// 将 HTML 颜色 #RRGGBB 转换为 ASS 格式 &H00BBGGRR
QString htmlColorToAss(const QString &htmlColor);

// 从文件名提取关键码，如 "aaa-304" → "aaa-304", "abc123" → "abc-123"
QString extractKey(const QString &fileName);
