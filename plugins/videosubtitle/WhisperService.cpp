#include "WhisperService.h"
#include "PluginLogger.h"
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QFile>
#include <QProcess>

WhisperService::WhisperService(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &WhisperService::onProcessTimeout);
}

void WhisperService::startTranscribe(const QString &whisperPath,
                                      const QString &modelPath,
                                      const QString &audioPath,
                                      const QString &outputPath,
                                      const QString &language,
                                      int segmentDuration)
{
    if (m_process) {
        cancel();
    }

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

    // Single whisper pass with -pp for real-time progress
    QFileInfo audioInfo(audioPath);
    QString stem = audioInfo.completeBaseName();
    QString outputFilePrefix = QDir(outputPath).filePath(stem);

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
         << "-of" << outputFilePrefix
         << "-pp";   // print progress — essential for real-time feedback

    if (!language.isEmpty() && language != "auto") {
        args << "-l" << language;
    }

    emit statusUpdate("正在加载模型...");
    m_process->start(whisperPath, args);

    // 空闲超时: 只要 whisper 还在打印进度就是活着的，计时器不断重置
    // 仅在完全静默 120 秒后才判定为卡死
    m_timer->start(120000);
    PluginLogger::info("Whisper 空闲超时: 120 秒（有进度输出自动续期）");
}

void WhisperService::cancel()
{
    m_timer->stop();
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

    // 收到进程输出 → 重置空闲超时（whisper 还在工作）
    m_timer->start(120000);

    // Read ALL available data — multiple progress lines may arrive in one chunk
    QString combined = QString::fromUtf8(m_process->readAllStandardOutput())
                     + QString::fromUtf8(m_process->readAllStandardError());

    // Use globalMatch to catch EVERY progress line, not just the first
    // whisper.cpp outputs lines like: "part progress = 53%"
    QRegularExpression progressRe(R"(part\s*progress\s*=\s*(\d+)%)");
    QRegularExpressionMatchIterator iter = progressRe.globalMatch(combined);

    // fallback: some whisper versions use "progress = XX%"
    if (!iter.hasNext()) {
        QRegularExpression fallbackRe(R"(progress\s*=\s*(\d+)%)");
        iter = fallbackRe.globalMatch(combined);
    }

    // Process each progress line individually so no virtual segment is skipped
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
    m_timer->stop();
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);

    // Locate the output SRT
    QString srtPath;
    if (success && !m_outputDir.isEmpty()) {
        QFileInfo audioInfo(m_audioInputPath);
        QString stem = audioInfo.completeBaseName();
        srtPath = QDir(m_outputDir).filePath(stem + ".srt");
        if (!QFileInfo::exists(srtPath)) {
            // Fallback: scan output directory
            QDir dir(m_outputDir);
            QFileInfoList srtFiles = dir.entryInfoList(QStringList("*.srt"), QDir::Files);
            if (!srtFiles.isEmpty())
                srtPath = srtFiles.last().absoluteFilePath();
        }
    }

    QString error;
    if (!success) {
        QString rawError = QString::fromUtf8(m_process->readAllStandardError());
        // 日志记录完整原始错误
        if (!rawError.isEmpty())
            PluginLogger::error("Whisper 原始错误: " + rawError);
        // 界面展示简明中文提示
        if (rawError.isEmpty()) {
            error = QString("语音识别工具无响应（退出码 %1）").arg(exitCode);
        } else if (rawError.contains("failed to load model") || rawError.contains("error loading model")) {
            error = "模型文件加载失败，请检查模型路径和格式";
        } else if (rawError.contains("cannot open")) {
            error = "无法打开音频文件，文件可能不存在或已被删除";
        } else if (rawError.contains("KEG") || rawError.contains("out of memory")) {
            error = "语音识别内存不足，请关闭其他程序后重试";
        } else if (rawError.contains("error: ")) {
            // 取 error: 后面的内容作为提示
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

    m_process->deleteLater();
    m_process = nullptr;

    emit progress(1.0);
    emit finished(success, srtPath, error);
}

void WhisperService::onProcessTimeout()
{
    PluginLogger::error("Whisper 进程超时，强制终止");
    emit statusUpdate("✗ 语音识别超时，进程已终止");

    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
        m_process->deleteLater();
        m_process = nullptr;
    }

    emit progress(1.0);
    emit finished(false, QString(), "语音识别超时，进程已终止");
}

// --- helper: read WAV header to get audio duration ---

bool WhisperService::isWhisperAvailable(const QString &whisperPath)
{
    if (whisperPath.isEmpty()) return false;

    QString nativePath = QDir::toNativeSeparators(whisperPath);

    QProcess proc;
    proc.start(nativePath, {"--help"});

    if (!proc.waitForStarted(3000)) {
        PluginLogger::warn(QString("Whisper 启动失败，可能缺少运行时 DLL: %1, 错误: %2")
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

    // WAV header layout at offset 28: byte rate (4 bytes, little-endian int32)
    // byte_rate = sample_rate * channels * bits_per_sample/8
    f.seek(28);
    QByteArray br = f.read(4);
    if (br.size() < 4) {
        f.close();
        return 0;
    }
    qint64 byteRate = *reinterpret_cast<const qint32*>(br.constData());
    if (byteRate <= 0) {
        f.close();
        return 0;
    }

    qint64 dataSize = f.size() - 44; // skip 44-byte WAV header
    f.close();

    if (dataSize <= 0) return 0;
    return dataSize * 1000 / byteRate;
}
