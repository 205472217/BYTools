#pragma once

#include <QObject>
#include <QProcess>

class WhisperService : public QObject
{
    Q_OBJECT
public:
    explicit WhisperService(QObject *parent = nullptr);

    void startTranscribe(const QString &whisperPath,
                         const QString &modelPath,
                         const QString &audioPath,
                         const QString &outputPath,
                         const QString &language = "auto");

    void cancel();

signals:
    void progress(double value);
    void finished(bool success, const QString &srtPath, const QString &error);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *m_process;
    QString m_outputDir;
    QString m_audioInputPath; // stored to derive deterministic output SRT path
};
