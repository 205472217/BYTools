#pragma once

#include <QObject>
#include <QTimer>
#include <QProcess>

class FFmpegService : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegService(QObject *parent = nullptr);

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

    void cancel();

    static bool isFFmpegAvailable(const QString &ffmpegPath);
    static QString ffmpegVersion(const QString &ffmpegPath);
    static qint64 getVideoDuration(const QString &ffmpegPath, const QString &videoPath);

signals:
    void progress(double value);
    void finished(bool success, const QString &outputPath, const QString &error);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessTimeout();

private:
    QProcess *m_process;
    QTimer *m_timer;
    QString m_outputPath;
    bool m_isExtracting = false;
    qint64 m_totalDuration = 0;
};
