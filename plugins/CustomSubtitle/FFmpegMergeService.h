#pragma once

#include <QObject>
#include <QThread>
#include <QAtomicInt>
#include "FfmpegRunner.h"

class PluginLogger;

class FFmpegMergeService : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegMergeService(PluginLogger *logger, QObject *parent = nullptr);
    ~FFmpegMergeService() override;

    Q_ENUM(GpuVendor)

    void startMerge(const QString &ffmpegPath,
                    const QString &videoDir,
                    const QString &outputDir,
                    bool recursive,
                    bool useGpu,
                    int quality = 0);

    void cancel();
    void requestStopAfterCount(int count);

signals:
    void progress(double value);
    void currentFileProgress(double value);   // 0.0~1.0 当前正在合成的单个视频进度
    void logMessage(const QString &message);
    void finished(bool success, const QString &error);
    void currentFileChanged(const QString &filePath);

private slots:
    void onRunnerProgress(double value);
    void onRunnerFinished(bool success, const QString &outputPath, const QString &error);
    void onCollectFinished();

private:
    void processNextFile();
    void doCollectFiles();

    struct VideoFile {
        QString path;
        QString subtitlePath;
        QString outputPath;
    };

    PluginLogger *m_logger = nullptr;
    FfmpegRunner *m_runner = nullptr;

    QString m_ffmpegPath;
    QString m_videoDir;
    QString m_outputDir;
    bool m_recursive = false;
    QList<VideoFile> m_pendingFiles;
    int m_currentIndex = -1;
    QAtomicInt m_cancelled{0};
    int m_stopTargetIndex = -1;   // ≥0 时处理到此索引后停止，含当前文件
    int m_successCount = 0;
    int m_failCount = 0;
    bool m_checkUseGpu = false;
    int m_quality = 0;

    QThread m_workerThread;
    bool m_workerRunning = false;
};
