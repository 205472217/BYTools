#include "SubtitleUtils.h"

QString htmlColorToAss(const QString &htmlColor)
{
    QString c = htmlColor.trimmed();
    if (c.startsWith('#'))
        c = c.mid(1);
    if (c.length() == 6) {
        // RRGGBB → AABBGGRR (AA=00 fully opaque)
        return QString("&H00%1%2%3")
            .arg(c.mid(4, 2))  // BB
            .arg(c.mid(2, 2))  // GG
            .arg(c.mid(0, 2)); // RR
    }
    return "&H00FFFFFF";
}
