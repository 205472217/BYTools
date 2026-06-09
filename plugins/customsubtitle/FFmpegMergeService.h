#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>

class FFmpegMergeService : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegMergeService(QObject *parent = nullptr);

    /// Start merging subtitle into video
    /// @param ffmpegPath  Path to ffmpeg executable
    /// @param videoDir    Directory containing original videos
    /// @param outputDir   Directory for merged output videos
    /// @param recursive   Scan subdirectories for videos
    /// @param useGpu      Enable GPU acceleration
    void startMerge(const QString &ffmpegPath,
                    const QString &videoDir,
                    const QString &outputDir,
                    bool recursive,
                    bool useGpu);

    void cancel();

    static bool isFFmpegAvailable(const QString &ffmpegPath);

signals:
    void progress(double value);
    void logMessage(const QString &message);
    void finished(bool success, const QString &error);
    void fileProcessed(const QString &videoName, bool ok, const QString &outputPath);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessTimeout();

private:
    void processNextFile();

    struct VideoFile {
        QString path;
        QString subtitlePath;
        QString outputPath;
    };

    QProcess *m_process = nullptr;
    QTimer *m_timer = nullptr;
    QString m_ffmpegPath;
    QString m_outputDir;
    QList<VideoFile> m_pendingFiles;
    int m_currentIndex = -1;
    bool m_cancelled = false;
    int m_successCount = 0;
    int m_failCount = 0;

    QStringList m_videoExts = {
        ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".ts"
    };
    QStringList m_subtitleExts = {".srt", ".ass", ".ssa"};
};
