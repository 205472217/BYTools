#include "WhisperService.h"
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

WhisperService::WhisperService(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
{
}

void WhisperService::startTranscribe(const QString &whisperPath,
                                      const QString &modelPath,
                                      const QString &audioPath,
                                      const QString &outputPath,
                                      const QString &language)
{
    if (m_process) {
        cancel();
    }

    m_outputDir = outputPath;
    m_audioInputPath = audioPath;

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &WhisperService::onProcessReadyRead);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &WhisperService::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &WhisperService::onProcessFinished);

    QStringList args;
    args << "-m" << modelPath
         << "-f" << audioPath
         << "-osrt"
         << "-o" << outputPath;

    if (!language.isEmpty() && language != "auto") {
        args << "-l" << language;
    }

    m_process->start(whisperPath, args);
}

void WhisperService::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void WhisperService::onProcessReadyRead()
{
    if (!m_process) return;

    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    QString errOutput = QString::fromUtf8(m_process->readAllStandardError());

    // whisper.cpp outputs progress like: "progress = 53%"
    QRegularExpression progressRe(R"(progress\s*=\s*(\d+)%)");
    QRegularExpressionMatch match = progressRe.match(output + errOutput);
    if (match.hasMatch()) {
        double progressValue = match.captured(1).toDouble() / 100.0;
        emit progress(qMin(1.0, progressValue));
    }
}

void WhisperService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);

    // Determine the expected SRT path deterministically
    // whisper.cpp -f <audioPath> -o <outputDir> -osrt produces:
    //   <outputDir>/<audioBasenameWithoutLastExt>.srt
    QString srtPath;
    if (success && !m_outputDir.isEmpty()) {
        QFileInfo audioInfo(m_audioInputPath);
        QString stem = audioInfo.completeBaseName(); // e.g. "video" from "video.wav"
        srtPath = QDir(m_outputDir).filePath(stem + ".srt");
        if (!QFileInfo::exists(srtPath)) {
            // Fallback: scan directory (edge case)
            QDir dir(m_outputDir);
            QStringList filters;
            filters << "*.srt";
            QFileInfoList srtFiles = dir.entryInfoList(filters, QDir::Files);
            if (!srtFiles.isEmpty()) {
                srtPath = srtFiles.last().absoluteFilePath();
            }
        }
    }

    QString error;
    if (!success) {
        error = QString::fromUtf8(m_process->readAllStandardError());
        if (error.isEmpty()) {
            error = QString("Whisper process exited with code %1").arg(exitCode);
        }
    }

    m_process->deleteLater();
    m_process = nullptr;

    emit progress(1.0);
    emit finished(success, srtPath, error);
}
