#pragma once

#include <QString>

// 将 HTML 颜色 #RRGGBB 转换为 ASS 格式 &H00BBGGRR
QString htmlColorToAss(const QString &htmlColor);
