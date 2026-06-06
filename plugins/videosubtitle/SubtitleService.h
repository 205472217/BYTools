#pragma once

#include <QString>
#include <QList>

class SubtitleService
{
public:
    struct SubtitleEntry {
        int index = 0;
        qint64 startTime = 0;   // milliseconds
        qint64 endTime = 0;
        QString originalText;
        QString translatedText;
    };

    // Parse SRT file
    static QList<SubtitleEntry> parseSrt(const QString &filePath);

    // Write SRT file
    static bool writeSrt(const QString &filePath,
                         const QList<SubtitleEntry> &entries);

    // Write ASS subtitle file (supports custom styles)
    static bool writeAss(const QString &filePath,
                         const QList<SubtitleEntry> &entries,
                         int fontSize = 20,
                         const QString &fontColor = "&H00FFFFFF",
                         const QString &borderColor = "&H00000000",
                         int borderWidth = 2);

    // Format milliseconds to SRT time format "00:01:23,456"
    static QString formatSrtTime(qint64 ms);

    // Format milliseconds to ASS time format "0:01:23.45"
    static QString formatAssTime(qint64 ms);

    // Extract plain text (remove index and time axis)
    static QStringList extractTexts(const QList<SubtitleEntry> &entries);

private:
    // Parse SRT time string "00:01:23,456" to milliseconds
    static qint64 parseSrtTime(const QString &timeStr);
};
