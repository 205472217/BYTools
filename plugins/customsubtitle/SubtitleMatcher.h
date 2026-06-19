#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QPair>
#include <QThread>
#include <QAtomicInt>

class PluginLogger;

class SubtitleMatcher : public QObject
{
    Q_OBJECT
public:
    explicit SubtitleMatcher(PluginLogger *logger, QObject *parent = nullptr);
    ~SubtitleMatcher();

    /// Extract key code from filename, e.g. "aaa-304" → "aaa-304", "abc123" → "abc-123"
    static QString extractKey(const QString &fileName);

    /// Result of a single match operation
    struct MatchResult {
        QString subtitlePath;       // original subtitle path
        QString subtitleName;       // original subtitle filename
        QString videoDir;           // target video directory
        QString newSubtitleName;    // renamed subtitle filename
        bool matched = false;
    };

    /// Start async operation: match → rename → move → preprocess
    void startMatchAsync(const QString &subtitleDir,
                          const QString &videoDir,
                          bool recursive,
                          const QStringList &videoExts,
                          const QStringList &preprocessors);

    void cancel();

signals:
    void logMessage(const QString &message);
    void progress(double value);
    void finished(bool success, const QString &error);
    void scanFinished(int matchedCount);

private:
    void doWork();
    QList<MatchResult> doMatch();

    // SRT preprocessing
    struct SrtEntry {
        int index = 0;
        qint64 startMs = 0;
        qint64 endMs = 0;
        QStringList textLines;
    };
    QList<SrtEntry> parseSrtFile(const QString &filePath);
    bool writeSrtFile(const QString &filePath, const QList<SrtEntry> &entries);
    void processSrtFile(const QString &filePath, const QStringList &ops);

    PluginLogger *m_logger;

    QThread m_workerThread;
    bool m_workerRunning = false;
    QAtomicInt m_cancelled{0};

    // Parameters for async operation
    QString m_subtitleDir;
    QString m_videoDir;
    bool m_recursive = false;
    QStringList m_videoExts;
    QStringList m_preprocessors;
};
