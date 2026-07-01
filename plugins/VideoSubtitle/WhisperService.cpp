#include "WhisperService.h"
#include "Logger.h"
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QFile>

WhisperService::WhisperService(PluginLogger *logger, QObject *parent)
    : ProcessRunner(parent)
{
    setLogger(logger);
}

void WhisperService::cancel()
{
    cancelProcess();
}

void WhisperService::startTranscribe(const QString &whisperPath,
                                      const QString &modelPath,
                                      const QString &audioPath,
                                      const QString &outputPath,
                                      const QString &language,
                                      int segmentDuration)
{
    m_outputDir = outputPath;
    m_audioInputPath = audioPath;
    m_segmentDuration = segmentDuration;
    m_virtualSegmentCount = 0;
    m_lastReportedSegment = -1;

    // Calculate virtual segment count from audio duration
    if (m_segmentDuration > 0) {
        qint64 durationMs = getWavDurationMs(audioPath);
        if (durationMs > 0) {
            m_virtualSegmentCount = qMax(1, static_cast<int>(
                (durationMs + m_segmentDuration * 1000 - 1) / (m_segmentDuration * 1000)));
            emit statusUpdate(QString("音频 %1 秒 → %2 段（每段 %3 秒）")
                .arg(durationMs / 1000)
                .arg(m_virtualSegmentCount)
                .arg(m_segmentDuration));
        }
    }

    QFileInfo audioInfo(audioPath);
    QString stem = audioInfo.completeBaseName();
    QString outputFilePrefix = QDir(outputPath).filePath(stem);

    QStringList args;
    args << "-m" << modelPath
         << "-f" << audioPath
         << "-osrt"
         << "-of" << outputFilePrefix
         << "-pp";

    if (!language.isEmpty() && language != "auto") {
        args << "-l" << language;
    }

    emit statusUpdate("正在加载模型...");
    startProcess(whisperPath, args, 120000, false, /*connectStdout=*/true);
    if (m_logger) m_logger->info("Whisper 空闲超时: 120 秒（有进度输出自动续期）");
}

// ── Protected overrides ──────────────────────────────────

void WhisperService::onStderrData(const QByteArray &data)
{
    QString text = QString::fromUtf8(data);

    // whisper.cpp outputs: "part progress = 53%"
    QRegularExpression progressRe(R"(part\s*progress\s*=\s*(\d+)%)");
    QRegularExpressionMatchIterator iter = progressRe.globalMatch(text);

    if (!iter.hasNext()) {
        QRegularExpression fallbackRe(R"(progress\s*=\s*(\d+)%)");
        iter = fallbackRe.globalMatch(text);
    }

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        double pct = qMin(1.0, match.captured(1).toDouble() / 100.0);
        emit progress(pct);

        if (m_virtualSegmentCount > 0) {
            int currentSeg = qMin(static_cast<int>(pct * m_virtualSegmentCount),
                                  m_virtualSegmentCount - 1);
            if (currentSeg == m_lastReportedSegment)
                continue;
            m_lastReportedSegment = currentSeg;
            emit statusUpdate(QString("→ 第 %1/%2 段 ... %3%")
                .arg(currentSeg + 1)
                .arg(m_virtualSegmentCount)
                .arg(static_cast<int>(pct * 100)));
        }
    }
}

void WhisperService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);

    QString srtPath;
    if (success && !m_outputDir.isEmpty()) {
        QFileInfo audioInfo(m_audioInputPath);
        QString stem = audioInfo.completeBaseName();
        srtPath = QDir(m_outputDir).filePath(stem + ".srt");
        if (!QFileInfo::exists(srtPath)) {
            QDir dir(m_outputDir);
            QFileInfoList srtFiles = dir.entryInfoList(QStringList("*.srt"), QDir::Files);
            if (!srtFiles.isEmpty())
                srtPath = srtFiles.last().absoluteFilePath();
        }
    }

    QString error;
    if (!success) {
        QString rawError = QString::fromUtf8(stderrBuffer());
        if (!rawError.isEmpty())
            if (m_logger) m_logger->error("Whisper 原始错误: " + rawError);

        if (rawError.isEmpty()) {
            error = QString("语音识别工具无响应（退出码 %1）").arg(exitCode);
        } else if (rawError.contains("failed to load model") || rawError.contains("error loading model")) {
            error = "模型文件加载失败，请检查模型路径和格式";
        } else if (rawError.contains("cannot open")) {
            error = "无法打开音频文件，文件可能不存在或已被删除";
        } else if (rawError.contains("KEG") || rawError.contains("out of memory")) {
            error = "语音识别内存不足，请关闭其他程序后重试";
        } else if (rawError.contains("error: ")) {
            int idx = rawError.indexOf("error: ");
            QString detail = rawError.mid(idx + 7, 80).trimmed();
            error = "语音识别出错: " + detail;
        } else {
            QString cleaned = rawError.trimmed();
            if (cleaned.length() > 100)
                cleaned = cleaned.left(100) + "...";
            error = "语音识别失败: " + cleaned;
        }
    }

    emit progress(1.0);
    emit finished(success, srtPath, error);
}

void WhisperService::onProcessTimeout()
{
    if (m_logger) m_logger->error("Whisper 进程超时，强制终止");
    emit statusUpdate("✗ 语音识别超时，进程已终止");
    cancelProcess();
    emit progress(1.0);
    emit finished(false, QString(), "语音识别超时，进程已终止");
}

// ── Private helpers ──────────────────────────────────────

bool WhisperService::isWhisperAvailable(const QString &whisperPath, PluginLogger *logger)
{
    if (whisperPath.isEmpty()) return false;

    QString nativePath = QDir::toNativeSeparators(whisperPath);

    QProcess proc;
    proc.start(nativePath, {"--help"});

    if (!proc.waitForStarted(3000)) {
        if (logger) logger->warn(QString("Whisper 启动失败，可能缺少运行时 DLL: %1, 错误: %2")
            .arg(nativePath, proc.errorString()));
        return false;
    }

    proc.kill();
    proc.waitForFinished(2000);
    return true;
}

qint64 WhisperService::getWavDurationMs(const QString &wavPath)
{
    QFile f(wavPath);
    if (!f.open(QIODevice::ReadOnly))
        return 0;

    f.seek(28);
    QByteArray br = f.read(4);
    if (br.size() < 4) {
        f.close();
        return 0;
    }
    qint32 byteRate = 0;
    memcpy(&byteRate, br.constData(), sizeof(byteRate));
    if (byteRate <= 0) {
        f.close();
        return 0;
    }

    qint64 dataSize = f.size() - 44;
    f.close();

    if (dataSize <= 0) return 0;
    return dataSize * 1000 / byteRate;
}
