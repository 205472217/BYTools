#pragma once

#include <QObject>
#include "FfmpegRunner.h"

class PluginLogger;

class FFmpegService : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegService(PluginLogger *logger, QObject *parent = nullptr);

    void startExtractAudio(const QString &ffmpegPath,
                           const QString &videoPath,
                           const QString &outputWav);

    void startBurnSubtitles(const QString &ffmpegPath,
                            const QString &videoPath,
                            const QString &srtPath,
                            const QString &outputPath,
                            int fontSize = 20,
                            const QString &fontColor = "#FFFFFF",
                            const QString &borderColor = "#000000",
                            int borderWidth = 2);

    void setUseHardwareAccel(bool enable);
    void cancel();

    static QString hardwareAccelName(const QString &ffmpegPath);

signals:
    void progress(double value);
    void finished(bool success, const QString &outputPath, const QString &error);

private slots:
    void onRunnerFinished(bool success, const QString &outputPath, const QString &error);

private:
    PluginLogger *m_logger = nullptr;
    FfmpegRunner *m_runner = nullptr;

    QString m_outputPath;
    bool m_isExtracting = false;
    bool m_checkUseGpu = false;
};
