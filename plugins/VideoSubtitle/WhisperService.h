#pragma once

#include <QObject>
#include <QProcess>
#include "ProcessRunner.h"

class PluginLogger;

class WhisperService : public ProcessRunner
{
    Q_OBJECT
public:
    explicit WhisperService(PluginLogger *logger, QObject *parent = nullptr);

    static bool isWhisperAvailable(const QString &whisperPath, PluginLogger *logger = nullptr);

    void cancel();

    void startTranscribe(const QString &whisperPath,
                         const QString &modelPath,
                         const QString &audioPath,
                         const QString &outputPath,
                         const QString &language = "auto",
                         int segmentDuration = 0);

signals:
    void progress(double value);
    void finished(bool success, const QString &srtPath, const QString &error);
    void statusUpdate(const QString &status);

protected:
    void onStderrData(const QByteArray &data) override;
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) override;
    void onProcessTimeout() override;

private:
    qint64 getWavDurationMs(const QString &wavPath);

    QString m_outputDir;
    QString m_audioInputPath;

    int m_segmentDuration = 0;
    int m_virtualSegmentCount = 0;
    int m_lastReportedSegment = -1;
};
