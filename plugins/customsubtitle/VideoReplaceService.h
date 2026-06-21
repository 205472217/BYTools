#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QAtomicInt>
#include <QMutex>

class PluginLogger;

class VideoReplaceService : public QObject
{
    Q_OBJECT
public:
    explicit VideoReplaceService(PluginLogger *logger, QObject *parent = nullptr);
    ~VideoReplaceService();

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
    /// Graceful stop: set cancel flag, let current item finish, then break
    void requestStop();

signals:
    void logMessage(const QString &message);
    void progress(double value);
    void finished(bool success, const QString &error);
    void scanFinished(int matchedCount);
    void currentFileChanged(const QString &filePath);

private:
    void doWork();

    struct ReplaceItem {
        QString originalPath;
        QString mergedPath;
        QString srtPath;       // may be empty
    };

    mutable QMutex m_mutex;

    PluginLogger *m_logger = nullptr;
    QString m_videoDir;
    QString m_mergedDir;
    bool m_recursive = false;
    bool m_removeSrt = true;
    bool m_backupOriginal = false;
    QAtomicInt m_cancelled{0};

    QList<ReplaceItem> m_items;
    int m_totalVideoCount = 0;
    int m_successCount = 0;
    int m_failCount = 0;

    QThread m_workerThread;
    bool m_workerRunning = false;
};
