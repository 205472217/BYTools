#include "SubtitleService.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

// Convert HTML color #RRGGBB to ASS format &H00BBGGRR
static QString htmlColorToAss(const QString &htmlColor)
{
    QString c = htmlColor.trimmed();
    if (c.startsWith('#'))
        c = c.mid(1);
    if (c.length() == 6) {
        return QString("&H00%1%2%3")
            .arg(c.mid(4, 2))  // BB
            .arg(c.mid(2, 2))  // GG
            .arg(c.mid(0, 2)); // RR
    }
    return "&H00FFFFFF";
}

QList<SubtitleService::SubtitleEntry> SubtitleService::parseSrt(const QString &filePath)
{
    QList<SubtitleEntry> entries;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return entries;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    QRegularExpression timeRegex(R"(^(\d{2}:\d{2}:\d{2},\d{3})\s*-->\s*(\d{2}:\d{2}:\d{2},\d{3}))");
    int currentIndex = 0;
    qint64 startTime = 0;
    qint64 endTime = 0;
    bool readingTime = false;
    QString textBuffer;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (line.isEmpty()) {
            if (readingTime && !textBuffer.isEmpty()) {
                SubtitleEntry entry;
                entry.index = ++currentIndex;
                entry.startTime = startTime;
                entry.endTime = endTime;
                entry.originalText = textBuffer.trimmed();
                entries.append(entry);
                textBuffer.clear();
            }
            readingTime = false;
            continue;
        }

        QRegularExpressionMatch match = timeRegex.match(line);
        if (match.hasMatch()) {
            readingTime = true;
            startTime = parseSrtTime(match.captured(1));
            endTime = parseSrtTime(match.captured(2));
        } else if (readingTime) {
            if (!textBuffer.isEmpty()) {
                textBuffer += "\n";
            }
            textBuffer += line;
        }
    }

    // Handle last entry
    if (readingTime && !textBuffer.isEmpty()) {
        SubtitleEntry entry;
        entry.index = ++currentIndex;
        entry.startTime = startTime;
        entry.endTime = endTime;
        entry.originalText = textBuffer.trimmed();
        entries.append(entry);
    }

    file.close();
    return entries;
}

bool SubtitleService::writeSrt(const QString &filePath,
                                const QList<SubtitleEntry> &entries,
                                bool bilingual)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    for (int i = 0; i < entries.size(); ++i) {
        const SubtitleEntry &entry = entries.at(i);
        stream << (i + 1) << "\n";
        stream << formatSrtTime(entry.startTime) << " --> " << formatSrtTime(entry.endTime) << "\n";

        if (bilingual && !entry.translatedText.isEmpty()) {
            stream << entry.originalText << "\n";
            stream << entry.translatedText << "\n";
        } else if (!entry.translatedText.isEmpty()) {
            stream << entry.translatedText << "\n";
        } else {
            stream << entry.originalText << "\n";
        }

        stream << "\n";
    }

    file.close();
    return true;
}

bool SubtitleService::writeAss(const QString &filePath,
                                const QList<SubtitleEntry> &entries,
                                int fontSize,
                                const QString &fontColor,
                                const QString &borderColor,
                                int borderWidth,
                                bool bilingual)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    // ASS header
    stream << "[Script Info]\n";
    stream << "ScriptType: v4.00+\n";
    stream << "PlayResX: 1920\n";
    stream << "PlayResY: 1080\n";
    stream << "WrapStyle: 0\n";
    stream << "\n";
    stream << "[V4+ Styles]\n";
    stream << "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n";

    // Convert HTML colors (#RRGGBB) to ASS format (&H00BBGGRR)
    QString assFontColor = htmlColorToAss(fontColor);
    QString assBorderColor = htmlColorToAss(borderColor);

    // Normal style
    stream << "Style: Default,Arial," << fontSize << "," << assFontColor
           << ",&H000000FF," << assBorderColor << ",&H80000000,"
           << "0,0,0,0,100,100,0,0,1," << borderWidth << ",1,2,10,10,40,1\n";

    // Translated style (if bilingual)
    if (bilingual) {
        int translatedFontSize = qMax(fontSize - 4, 12);
        stream << "Style: Translated,Arial," << translatedFontSize << "," << assFontColor
               << ",&H000000FF," << assBorderColor << ",&H80000000,"
               << "0,0,0,0,100,100,0,0,1," << borderWidth << ",1,2,10,10,10,1\n";
    }

    stream << "\n";
    stream << "[Events]\n";
    stream << "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n";

    // Dialogue lines
    for (const SubtitleEntry &entry : entries) {
        QString startStr = formatAssTime(entry.startTime);
        QString endStr = formatAssTime(entry.endTime);

        // Original text
        stream << "Dialogue: 0," << startStr << "," << endStr
               << ",Default,,0,0,0,," << entry.originalText << "\n";

        // Translated text
        if (bilingual && !entry.translatedText.isEmpty()) {
            stream << "Dialogue: 0," << startStr << "," << endStr
                   << ",Translated,,0,0,0,," << entry.translatedText << "\n";
        }
    }

    file.close();
    return true;
}

QString SubtitleService::formatSrtTime(qint64 ms)
{
    qint64 hours = ms / 3600000;
    ms %= 3600000;
    qint64 minutes = ms / 60000;
    ms %= 60000;
    qint64 seconds = ms / 1000;
    qint64 millis = ms % 1000;

    return QString("%1:%2:%3,%4")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(millis, 3, 10, QChar('0'));
}

QString SubtitleService::formatAssTime(qint64 ms)
{
    qint64 hours = ms / 3600000;
    ms %= 3600000;
    qint64 minutes = ms / 60000;
    ms %= 60000;
    qint64 seconds = ms / 1000;
    qint64 centiseconds = (ms % 1000) / 10;

    return QString("%1:%2:%3.%4")
        .arg(hours)
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(centiseconds, 2, 10, QChar('0'));
}

QStringList SubtitleService::extractTexts(const QList<SubtitleEntry> &entries)
{
    QStringList texts;
    for (const SubtitleEntry &entry : entries) {
        texts.append(entry.originalText);
    }
    return texts;
}

qint64 SubtitleService::parseSrtTime(const QString &timeStr)
{
    // Parse "00:01:23,456" to milliseconds
    QRegularExpression re(R"((\d{2}):(\d{2}):(\d{2})[,.](\d{3}))");
    QRegularExpressionMatch match = re.match(timeStr);
    if (!match.hasMatch()) return 0;

    qint64 hours = match.captured(1).toLongLong();
    qint64 minutes = match.captured(2).toLongLong();
    qint64 seconds = match.captured(3).toLongLong();
    qint64 millis = match.captured(4).toLongLong();

    return hours * 3600000 + minutes * 60000 + seconds * 1000 + millis;
}
