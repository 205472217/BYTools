#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class VideoReplaceService : public QObject
{
    Q_OBJECT
public:
    explicit VideoReplaceService(QObject *parent = nullptr);

    /// Step 4: Match merged videos in outputDir against originals in videoDir,
    /// replace originals, clean up subtitles.
    /// @param videoDir    Directory of original video files
    /// @param mergedDir   Directory of merged (burned) video files
    /// @param recursive   Whether to scan subdirectories in videoDir
    /// @param removeSrt   Whether to delete matching .srt files after replacement
    /// @param backupOriginal  Whether to backup original files before replacement
    void startReplace(const QString &videoDir,
                      const QString &mergedDir,
                      bool recursive,
                      bool removeSrt,
                      bool backupOriginal);

    void cancel();

signals:
    void logMessage(const QString &message);
    void progress(double value);
    void finished(bool success, const QString &error);

private:
    void processNextFile();

    struct ReplaceItem {
        QString originalPath;
        QString mergedPath;
        QString srtPath;       // may be empty
    };

    QString m_videoDir;
    QString m_mergedDir;
    bool m_recursive = false;
    bool m_removeSrt = true;
    bool m_backupOriginal = false;
    bool m_cancelled = false;

    QList<ReplaceItem> m_items;
    int m_currentIndex = -1;
    int m_successCount = 0;
    int m_failCount = 0;

    QStringList m_videoExts = {
        ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".ts"
    };
};
