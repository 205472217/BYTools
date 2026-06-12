#pragma once

#include <QObject>
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
                    bool useFragMp4 = true);

    void cancel();
    void requestStopAfterCurrent();

signals:
    void progress(double value);
    void currentFileProgress(double value);   // 0.0~1.0 当前正在合成的单个视频进度
    void logMessage(const QString &message);
    void finished(bool success, const QString &error);
    void currentFileChanged(const QString &filePath);

private slots:
    void onRunnerProgress(double value);
    void onRunnerFinished(bool success, const QString &outputPath, const QString &error);

private:
    void processNextFile();

    struct VideoFile {
        QString path;
        QString subtitlePath;
        QString outputPath;
    };

    PluginLogger *m_logger = nullptr;
    FfmpegRunner *m_runner = nullptr;

    QString m_ffmpegPath;
    QString m_outputDir;
    QList<VideoFile> m_pendingFiles;
    int m_currentIndex = -1;
    bool m_cancelled = false;
    bool m_stopAfterCurrent = false;
    int m_successCount = 0;
    int m_failCount = 0;
    bool m_checkUseGpu = false;
    bool m_useFragMp4 = true;
};
