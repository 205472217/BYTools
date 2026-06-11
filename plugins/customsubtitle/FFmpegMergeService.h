#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>

class FFmpegMergeService : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegMergeService(QObject *parent = nullptr);

    /// GPU vendor auto-detected from ffmpeg
    enum class GpuVendor {
        None,
        CUDA,   // NVIDIA
        AMD,    // AMD
        Intel   // Intel QSV
    };
    Q_ENUM(GpuVendor)

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

    /// Stop after the current processing file finishes (graceful stop)
    void requestStopAfterCurrent();

    static bool isFFmpegAvailable(const QString &ffmpegPath);

    /// Auto-detect available GPU encoder in ffmpeg (CUDA → AMD → Intel)
    static GpuVendor detectGpuVendor(const QString &ffmpegPath);

    /// Convert GpuVendor to human-readable string
    static QString gpuVendorToString(GpuVendor vendor);

signals:
    void progress(double value);
    void logMessage(const QString &message);
    void finished(bool success, const QString &error);
    void currentFileChanged(const QString &filePath);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessTimeout();

private:
    void processNextFile();
    void buildFfmpegArgs(const QString &videoPath, const QString &subtitlePath,
                         const QString &outputPath, QStringList &args);

    /// Build GPU-accelerated ffmpeg args (with hwdownload/hwupload for subtitles filter)
    QStringList buildGpuAccelArgs(const QString &videoPath, const QString &subtitlePath,
                                  const QString &outputPath);

    /// Detect input video codec (h264 / hevc)
    static QString detectInputCodec(const QString &ffmpegPath, const QString &videoPath);

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
    bool m_stopAfterCurrent = false;
    int m_successCount = 0;
    int m_failCount = 0;
    GpuVendor m_gpuVendor = GpuVendor::None;
    bool m_useGpu = false;
    bool m_burnFallbackTried = false;

    /// 缓存最近一次 stderr 输出（onProcessFinished 用它获取错误信息）
    QByteArray m_lastStderrBuffer;

    /// Total duration of current video for progress calculation
    qint64 m_totalDuration = 0;

    struct BurnParams {
        QString ffmpegPath;
        QString videoPath;
        QString subtitlePath;
        QString outputPath;
    };
    BurnParams m_burnParams;

    QStringList m_videoExts = {
        ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".ts"
    };
    QStringList m_subtitleExts = {".srt", ".ass", ".ssa"};
};
