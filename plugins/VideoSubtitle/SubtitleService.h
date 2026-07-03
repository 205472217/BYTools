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
                         int fontSize = 18,
                         const QString &fontColor = "&H00FFFFFF",
                         const QString &borderColor = "&H00000000",
                         int borderWidth = 2);

    // Format milliseconds to SRT time format "00:01:23,456"
    static QString formatSrtTime(qint64 ms);

    // Format milliseconds to ASS time format "0:01:23.45"
    static QString formatAssTime(qint64 ms);

    // Deduplicate consecutive duplicate text entries (keep first occurrence)
    static QList<SubtitleEntry> deduplicate(const QList<SubtitleEntry> &entries);

    // Filter out environment sound entries (e.g., "(music)", "[Applause]", "（掌声）")
    // These non-dialog entries are removed completely (text + timestamps)
    static QList<SubtitleEntry> filterEnvironmentSounds(const QList<SubtitleEntry> &entries);

    // Check if text is a music entry wrapped in ♪ ... ♪ (e.g., "♪ Happy Birthday ♪")
    // Music entries should be preserved as-is and not sent for translation
    static bool isMusicText(const QString &text);

    // Check if text is a sound effect wrapped in * ... * (e.g., "*叮咚*", "*门铃声*")
    // These non-dialog entries should be removed completely
    static bool isSoundEffect(const QString &text);

    // Detect language of subtitle entries by analyzing character ranges
    // Returns "en", "ja", "zh", "ko", or "auto" (for mixed/unrecognized)
    static QString detectLanguage(const QList<SubtitleEntry> &entries);

private:
    // Parse SRT time string "00:01:23,456" to milliseconds
    static qint64 parseSrtTime(const QString &timeStr);
};
