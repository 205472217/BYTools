#include "FfmpegRunner.h"
#include "Logger.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <QThread>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

FfmpegRunner::FfmpegRunner(QObject *parent)
    : ProcessRunner(parent)
{
}

void FfmpegRunner::cancel()
{
    cancelProcess();
}

// ── Public API ────────────────────────────────────────────

void FfmpegRunner::burnSubtitles(const BurnConfig &config)
{
    m_operation = Operation::Burn;
    m_burnConfig = config;
    m_burnFallbackTried = false;
    m_audioOutputPath.clear();

    // 自动检测 GPU（如果调用者没预先检测）
    GpuVendor vendor = GpuVendor::None;
    if (config.useGpu) {
        vendor = detectGpuVendor(config.ffmpegPath);
    }

    // 获取视频时长用于进度计算
    m_totalDuration = getVideoDuration(config.ffmpegPath, config.videoPath);

    // 获取原视频码率（仅视频流，不含音频 — 用于 VBR 目标码率保持文件大小）
    m_srcBitrate = getVideoStreamBitrate(config.ffmpegPath, config.videoPath);
    qint64 srcBitrate = m_srcBitrate;
    bool hasBitrate = (srcBitrate > 0);

    // 计算线程限制：默认保留 ~15% CPU 余量，避免系统卡顿
    if (m_burnConfig.threadCount == 0) {
        m_burnConfig.threadCount = qMax(1, qRound(QThread::idealThreadCount() * 0.85));
    }

    QStringList args;

    if (config.useGpu && vendor != GpuVendor::None) {
        QString codec = detectInputCodec(config.ffmpegPath, config.videoPath);
        QString filter = buildSubtitleFilter(config.subtitlePath, config.fontName,
                                              config.fontSize, config.fontColor,
                                              config.borderColor, config.borderWidth);
        args = buildGpuAccelArgs(vendor, config.videoPath, filter,
                                  config.outputPath, codec, srcBitrate);
    }

    if (args.isEmpty()) {
        // 软件编码 — 目标码率 = 源码率，峰值不超过 1.5 倍
        args << "-threads" << QString::number(m_burnConfig.threadCount)
             << "-i" << config.videoPath
             << "-vf" << buildSubtitleFilter(config.subtitlePath, config.fontName,
                                              config.fontSize, config.fontColor,
                                              config.borderColor, config.borderWidth)
             << "-c:v" << "libx264"
             << "-preset" << "slow";
        if (hasBitrate) {
            args << "-b:v" << QString::number(srcBitrate)
                 << "-maxrate" << QString::number(srcBitrate * 3 / 2)   // ×1.5 峰值上限
                 << "-bufsize" << QString::number(srcBitrate * 3);       // 3x 缓冲
        } else {
            args << "-b:v" << "5M"
                 << "-maxrate" << "10M"
                 << "-bufsize" << "20M";
        }
        args << "-c:a" << "copy"
             << "-y";
        args << config.outputPath;
    } else {
        // GPU 路径：在 -i 前插入线程限制
        args.prepend(QString::number(m_burnConfig.threadCount));
        args.prepend("-threads");
    }

    if (m_logger) m_logger->info("FfmpegRunner burn args: " + args.join(" "));
    startProcess(config.ffmpegPath, args, 120000, /*graceful=*/true, /*connectStdout=*/false);

    // 设置 ffmpeg 进程为低优先级，避免抢占系统资源
#ifdef Q_OS_WIN
    connect(m_process, &QProcess::started, this, [this]() {
        if (!m_process)
            return;
        HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE,
                                       (DWORD)m_process->processId());
        if (hProcess) {
            SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS);
            CloseHandle(hProcess);
        }
    }, Qt::SingleShotConnection);
#endif
}

void FfmpegRunner::extractAudio(const ExtractAudioConfig &config)
{
    m_operation = Operation::ExtractAudio;
    m_audioOutputPath = config.outputWavPath;

    QStringList args;
    args << "-i" << config.videoPath
         << "-vn"
         << "-acodec" << "pcm_s16le"
         << "-ar" << QString::number(config.sampleRate)
         << "-ac" << QString::number(config.channels)
         << "-y"
         << config.outputWavPath;

    if (m_logger) m_logger->info("FfmpegRunner extractAudio args: " + args.join(" "));
    startProcess(config.ffmpegPath, args, 120000, /*graceful=*/false, /*connectStdout=*/false);
}

// ── Protected overrides ──────────────────────────────────

void FfmpegRunner::onStderrData(const QByteArray &data)
{
    if (m_operation != Operation::Burn || m_totalDuration <= 0)
        return;

    QString output = QString::fromLocal8Bit(data);
    QRegularExpression timeRe(R"(time=(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    QRegularExpressionMatch match = timeRe.match(output);
    if (match.hasMatch()) {
        qint64 h = match.captured(1).toLongLong();
        qint64 m = match.captured(2).toLongLong();
        qint64 s = match.captured(3).toLongLong();
        qint64 cs = match.captured(4).toLongLong();
        qint64 currentMs = (h * 3600 + m * 60 + s) * 1000 + cs * 10;
        double p = qMin(1.0, static_cast<double>(currentMs) / m_totalDuration);
        emit progress(p);
    }
}

void FfmpegRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    QString error;

    if (m_operation == Operation::Burn) {
        if (!success) {
            QString rawError = QString::fromLocal8Bit(stderrBuffer());
            if (m_logger) m_logger->error("FfmpegRunner burn failed: " + rawError);

            // GPU 加速失败 → 自动回退到软件编码（仅本次操作）
            if (m_burnConfig.useGpu && !m_burnFallbackTried) {
                m_burnFallbackTried = true;
                if (m_logger) m_logger->info("FfmpegRunner: GPU burn failed, retrying with CPU...");

                // 用 CPU 软件编码重新烧录（不改变 config，避免递归）
                BurnConfig swConfig = m_burnConfig;
                swConfig.useGpu = false;
                swConfig.gpuVendor = GpuVendor::None;

                QStringList swArgs;
                swArgs << "-threads" << QString::number(m_burnConfig.threadCount)
                       << "-i" << swConfig.videoPath
                       << "-vf" << buildSubtitleFilter(swConfig.subtitlePath, swConfig.fontName,
                                                        swConfig.fontSize, swConfig.fontColor,
                                                        swConfig.borderColor, swConfig.borderWidth)
                       << "-c:v" << "libx264"
                       << "-preset" << "slow";
                if (m_srcBitrate > 0) {
                    swArgs << "-b:v" << QString::number(m_srcBitrate)
                           << "-maxrate" << QString::number(m_srcBitrate * 3 / 2)
                           << "-bufsize" << QString::number(m_srcBitrate * 3);
                } else {
                    swArgs << "-b:v" << "5M"
                           << "-maxrate" << "10M"
                           << "-bufsize" << "20M";
                }
                swArgs << "-c:a" << "copy"
                       << "-y"
                       << swConfig.outputPath;

                if (m_logger) m_logger->info("FfmpegRunner CPU fallback args: " + swArgs.join(" "));
                startProcess(swConfig.ffmpegPath, swArgs, 120000, /*graceful=*/true);

                // 同样设置低优先级
#ifdef Q_OS_WIN
                connect(m_process, &QProcess::started, this, [this]() {
                    if (!m_process)
                        return;
                    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE,
                                                   (DWORD)m_process->processId());
                    if (hProcess) {
                        SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS);
                        CloseHandle(hProcess);
                    }
                }, Qt::SingleShotConnection);
#endif
                return;
            }

            error = extractFfmpegError(rawError);
            if (error.isEmpty())
                error = QString("退出码=%1").arg(exitCode);
        }

        emit progress(1.0);
        emit finished(success, success ? m_burnConfig.outputPath : QString(), error);

    } else if (m_operation == Operation::ExtractAudio) {
        if (!success) {
            QString rawError = QString::fromLocal8Bit(stderrBuffer());
            if (m_logger) m_logger->error("FfmpegRunner extractAudio failed: " + rawError);

            if (rawError.contains("Output file does not contain any stream")) {
                error = "视频中没有音频轨道，无法提取音频";
            } else if (rawError.contains("Invalid data found when processing input")) {
                error = "无法读取视频文件，文件可能已损坏或格式不支持";
            } else if (rawError.contains("No such file or directory")) {
                error = "视频文件不存在或路径错误";
            } else if (rawError.contains("Permission denied")) {
                error = "没有权限访问视频文件";
            } else {
                error = extractFfmpegError(rawError);
                if (error.isEmpty())
                    error = "处理视频时发生未知错误";
            }
        }

        emit progress(1.0);
        emit finished(success, success ? m_audioOutputPath : QString(), error);
    }
}

void FfmpegRunner::onProcessTimeout()
{
    QString type = (m_operation == Operation::ExtractAudio) ? "音频提取" : "烧录字幕";
    if (m_logger) m_logger->error(QString("FfmpegRunner: %1 超时").arg(type));

    // 基类 cancelProcess() 会优雅退出（写 q\n，等 15s，强杀）
    ProcessRunner::onProcessTimeout();

    QString outputPath;
    if (m_operation == Operation::Burn)
        outputPath = m_burnConfig.outputPath;
    else
        outputPath = m_audioOutputPath;

    emit progress(1.0);
    emit finished(false, outputPath, QString("%1 超时，进程已终止").arg(type));
}
