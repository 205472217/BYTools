#pragma once

#include <QObject>
#include <QTimer>
#include <QProcess>

class WhisperService : public QObject
{
    Q_OBJECT
public:
    explicit WhisperService(QObject *parent = nullptr);

    // 尝试启动 whisper-cli.exe 验证其运行时依赖（DLL）是否完整
    static bool isWhisperAvailable(const QString &whisperPath);

    // segmentDuration: seconds per virtual segment, 0 = no segmentation
    void startTranscribe(const QString &whisperPath,
                         const QString &modelPath,
                         const QString &audioPath,
                         const QString &outputPath,
                         const QString &language = "auto",
                         int segmentDuration = 0);

    void cancel();

signals:
    void progress(double value);
    void finished(bool success, const QString &srtPath, const QString &error);
    void statusUpdate(const QString &status);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessTimeout();

private:
    qint64 getWavDurationMs(const QString &wavPath);

    QProcess *m_process = nullptr;
    QTimer *m_timer = nullptr;
    QString m_outputDir;
    QString m_audioInputPath;

    // Virtual segment tracking (no physical audio splitting)
    int m_segmentDuration = 0;      // seconds per virtual segment, 0 = disabled
    int m_virtualSegmentCount = 0;
    int m_lastReportedSegment = -1;
};
