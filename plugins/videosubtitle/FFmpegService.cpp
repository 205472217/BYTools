#include "FFmpegService.h"
#include "PluginLogger.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <QDir>

// Helper: convert HTML color #RRGGBB to ASS color &H00BBGGRR
static QString htmlColorToAss(const QString &htmlColor)
{
    QString c = htmlColor.trimmed();
    if (c.startsWith('#'))
        c = c.mid(1);
    if (c.length() == 6) {
        // RRGGBB → AABBGGRR (AA=00 fully opaque)
        return QString("&H00%1%2%3")
            .arg(c.mid(4, 2))  // BB
            .arg(c.mid(2, 2))  // GG
            .arg(c.mid(0, 2)); // RR
    }
    return "&H00FFFFFF";
}

FFmpegService::FFmpegService(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_timer(new QTimer(this))
    , m_isExtracting(false)
    , m_useHardwareAccel(false)
    , m_totalDuration(0)
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &FFmpegService::onProcessTimeout);
}

void FFmpegService::setUseHardwareAccel(bool enable)
{
    m_useHardwareAccel = enable;
}

QString FFmpegService::detectInputCodec(const QString &ffmpegPath, const QString &videoPath)
{
    if (ffmpegPath.isEmpty() || videoPath.isEmpty())
        return "h264";

    QProcess proc;
    proc.start(ffmpegPath, {"-i", videoPath});
    if (!proc.waitForFinished(10000))
        return "h264";

    QString output = QString::fromUtf8(proc.readAllStandardError());
    // Stream #0:0[0x1](und): Video: h264 (High) (avc1 / 0x31637661), ...
    // Stream #0:0: Video: hevc (Main), ...
    QRegularExpression re(R"(Stream\s+#0:0.*Video:\s*(\w+))");
    QRegularExpressionMatch m = re.match(output);
    if (m.hasMatch()) {
        QString codec = m.captured(1).toLower();
        if (codec == "h264" || codec == "avc" || codec == "avc1")
            return "h264";
        if (codec == "hevc" || codec == "h265" || codec == "hevc1")
            return "hevc";
        if (codec == "av1")
            return "av1";
    }
    return "h264";  // default fallback
}

int FFmpegService::detectHardwareAccel(const QString &ffmpegPath)
{
    if (ffmpegPath.isEmpty())
        return 0;

    // 1) 先查 FFmpeg 编译了哪些 GPU 编码器
    QProcess proc;
    proc.start(ffmpegPath, {"-encoders"});
    if (!proc.waitForFinished(15000))
        return 0;

    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
    QString outputUpper = output.toUpper();

    bool hasNVENC = outputUpper.contains("NVENC");
    bool hasQSV   = outputUpper.contains("QSV");
    bool hasAMF   = outputUpper.contains("AMF");

    // 2) 按优先级逐一验证硬件是否真实存在
    auto probeDevice = [&](const QStringList &args) -> bool {
        QProcess p;
        p.start(ffmpegPath, args);
        p.waitForFinished(5000);
        return (p.exitCode() == 0);
    };

    // NVIDIA — 尝试初始化 CUDA 设备（检查 nvcuda.dll 是否存在）
    if (hasNVENC && probeDevice({"-init_hw_device", "cuda=probe", "-f", "null", "-"}))
        return 1;

    // AMD AMF — 尝试用 h264_amf 编码一帧黑屏（确认驱动正常）
    if (hasAMF) {
        QProcess p;
        p.start(ffmpegPath, {"-f", "lavfi", "-i", "color=c=black:s=64x64:d=0.1",
                             "-c:v", "h264_amf", "-f", "null", "-"});
        p.waitForFinished(5000);
        if (p.exitCode() == 0)
            return 3;
    }

    // Intel QSV — 尝试初始化 QSV 设备
    if (hasQSV && probeDevice({"-init_hw_device", "qsv=probe", "-f", "null", "-"}))
        return 2;

    return 0;  // 没有任何 GPU 编码器可用
}

QString FFmpegService::hardwareAccelName(const QString &ffmpegPath)
{
    switch (detectHardwareAccel(ffmpegPath)) {
    case 1: return "NVIDIA NVENC";
    case 2: return "Intel QSV";
    case 3: return "AMD AMF";
    default: return "未检测到 GPU 编码器";
    }
}

QStringList FFmpegService::buildGpuArgs(const QString &ffmpegPath,
                                         const QString &videoPath,
                                         const QString &styleFilter,
                                         const QString &outputPath)
{
    QStringList args;
    // 确保检测到 GPU 类型后才调用此函数
    int gpuType = detectHardwareAccel(ffmpegPath);
    QString codec = detectInputCodec(ffmpegPath, videoPath);

    // 根据输入编码选对应的 GPU 编码器
    auto gpuEncoder = [&](const QString &nvidia, const QString &intel, const QString &amd) -> QString {
        if (codec == "hevc") {
            switch (gpuType) {
            case 1: return "hevc_" + nvidia;
            case 2: return "hevc_" + intel;
            case 3: return "hevc_" + amd;
            }
        }
        switch (gpuType) {
        case 1: return "h264_" + nvidia;
        case 2: return "h264_" + intel;
        case 3: return "h264_" + amd;
        }
        return QString();
    };

    switch (gpuType) {
    case 1: {
        // NVIDIA CUDA / NVENC
        QString encoder = gpuEncoder("nvenc", "qsv", "amf");
        args << "-hwaccel" << "cuda"
             << "-hwaccel_output_format" << "cuda"
             << "-i" << videoPath
             << "-vf" << ("hwdownload,format=nv12," + styleFilter + ",hwupload_cuda")
             << "-c:v" << encoder
             << "-preset" << "p7"
             << "-cq" << "23"
             << "-c:a" << "copy"
             << "-y"
             << outputPath;
        break;
    }
    case 2: {
        // Intel QSV
        QString encoder = gpuEncoder("nvenc", "qsv", "amf");
        args << "-hwaccel" << "qsv"
             << "-hwaccel_output_format" << "qsv"
             << "-i" << videoPath
             << "-vf" << ("hwdownload=format=nv12," + styleFilter + ",hwupload=format=nv12")
             << "-c:v" << encoder
             << "-preset" << "veryfast"
             << "-global_quality" << "23"
             << "-c:a" << "copy"
             << "-y"
             << outputPath;
        break;
    }
    case 3: {
        // AMD AMF
        QString encoder = gpuEncoder("nvenc", "qsv", "amf");
        args << "-hwaccel" << "dxva2"
             << "-i" << videoPath
             << "-vf" << styleFilter
             << "-c:v" << encoder
             << "-quality" << "quality"
             << "-c:a" << "copy"
             << "-y"
             << outputPath;
        break;
    }
    }
    return args;
}

void FFmpegService::startExtractAudio(const QString &ffmpegPath,
                                       const QString &videoPath,
                                       const QString &outputWav)
{
    if (m_process) {
        cancel();
    }

    m_outputPath = outputWav;
    m_isExtracting = true;

    // Get total duration for progress
    m_totalDuration = getVideoDuration(ffmpegPath, videoPath);

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &FFmpegService::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FFmpegService::onProcessFinished);

    QStringList args;
    args << "-i" << videoPath
         << "-vn"
         << "-acodec" << "pcm_s16le"
         << "-ar" << "16000"
         << "-ac" << "1"
         << "-y"
         << outputWav;

    m_process->start(ffmpegPath, args);

    // 空闲超时: 只要 ffmpeg 还在输出进度就是活着的，计时器不断重置
    m_timer->start(120000);
    PluginLogger::info("FFmpeg 音频提取空闲超时: 120 秒（有进度输出自动续期）");
}

void FFmpegService::startBurnSubtitles(const QString &ffmpegPath,
                                        const QString &videoPath,
                                        const QString &srtPath,
                                        const QString &outputPath,
                                        int fontSize,
                                        const QString &fontColor,
                                        const QString &borderColor,
                                        int borderWidth)
{
    if (m_process) {
        cancel();
    }

    m_outputPath = outputPath;
    m_isExtracting = false;

    m_totalDuration = getVideoDuration(ffmpegPath, videoPath);

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &FFmpegService::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FFmpegService::onProcessFinished);

    // Build subtitle filter with full style
    // 必须用 \\: 双反斜杠，ffmpeg 4.x graph parser 先吃一层再传给 av_set_options_string
    QString escapedSrtPath = srtPath;
    escapedSrtPath.replace("\\", "/");
    escapedSrtPath.replace(":", "\\\\:");
    QString assFontColor = htmlColorToAss(fontColor);
    QString assBorderColor = htmlColorToAss(borderColor);
    QString styleFilter =
        QString("subtitles='%1':force_style='FontSize=%2,PrimaryColour=%3,OutlineColour=%4,BorderStyle=1,Outline=%5,Shadow=1'")
            .arg(escapedSrtPath,
                 QString::number(fontSize),
                 assFontColor,
                 assBorderColor,
                 QString::number(borderWidth));

    QStringList args;
    if (m_useHardwareAccel) {
        args = buildGpuArgs(ffmpegPath, videoPath, styleFilter, outputPath);
        if (args.isEmpty()) {
            PluginLogger::warn("GPU 加速不可用（未检测到 GPU 编码器），回退到软件编码");
        }
    }
    if (args.isEmpty()) {
        // 软件编码（原始行为）
        args << "-i" << videoPath
             << "-vf" << styleFilter
             << "-c:a" << "copy"
             << "-y"
             << outputPath;
    } else {
        PluginLogger::info("使用 GPU 硬件加速烧录字幕");
    }

    // 缓存参数供 GPU 失败回退使用
    m_burnParams = {ffmpegPath, videoPath, srtPath, outputPath, fontSize, fontColor, borderColor, borderWidth};
    m_burnFallbackTried = false;

    m_process->start(ffmpegPath, args);

    // 空闲超时: 只要 ffmpeg 还在输出进度就是活着的，计时器不断重置
    m_timer->start(120000);
    PluginLogger::info("FFmpeg 烧录空闲超时: 120 秒（有进度输出自动续期）");
}

void FFmpegService::cancel()
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

bool FFmpegService::isFFmpegAvailable(const QString &ffmpegPath)
{
    if (ffmpegPath.isEmpty()) return false;

    // 在 Windows 上使用原生反斜杠路径，避免 QProcess 解析问题
    QString nativePath = QDir::toNativeSeparators(ffmpegPath);

    QProcess proc;
    proc.start(nativePath, {"-version"});

    // 检查是否成功启动
    if (!proc.waitForStarted(3000)) {
        PluginLogger::warn(QString("FFmpeg 启动失败: %1, 错误: %2")
            .arg(nativePath, proc.errorString()));
        return false;
    }

    if (!proc.waitForFinished(5000)) {
        // 超时，强制终止
        proc.kill();
        proc.waitForFinished(2000);
        PluginLogger::warn(QString("FFmpeg 执行超时: %1").arg(nativePath));
        return false;
    }

    bool success = (proc.exitCode() == 0);
    if (!success) {
        PluginLogger::warn(QString("FFmpeg 返回值非零: %1, exitCode=%2, stderr=%3")
            .arg(nativePath)
            .arg(proc.exitCode())
            .arg(QString::fromUtf8(proc.readAllStandardError()).trimmed()));
    }
    return success;
}

QString FFmpegService::ffmpegVersion(const QString &ffmpegPath)
{
    if (ffmpegPath.isEmpty()) return QString();

    QString nativePath = QDir::toNativeSeparators(ffmpegPath);

    QProcess proc;
    proc.start(nativePath, {"-version"});

    if (!proc.waitForStarted(3000) ||
        !proc.waitForFinished(5000) ||
        proc.exitCode() != 0) {
        return QString();
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QRegularExpression re(R"(ffmpeg version (\S+))");
    QRegularExpressionMatch match = re.match(output);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return QString();
}

qint64 FFmpegService::getVideoDuration(const QString &ffmpegPath, const QString &videoPath)
{
    if (ffmpegPath.isEmpty() || videoPath.isEmpty()) return 0;

    QProcess proc;
    proc.start(ffmpegPath, {"-i", videoPath});
    proc.waitForFinished(10000);

    QString output = QString::fromUtf8(proc.readAllStandardError());
    QRegularExpression re(R"(Duration:\s*(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    QRegularExpressionMatch match = re.match(output);
    if (!match.hasMatch()) return 0;

    qint64 hours = match.captured(1).toLongLong();
    qint64 minutes = match.captured(2).toLongLong();
    qint64 seconds = match.captured(3).toLongLong();
    qint64 centiseconds = match.captured(4).toLongLong();

    return (hours * 3600 + minutes * 60 + seconds) * 1000 + centiseconds * 10;
}

void FFmpegService::onProcessReadyRead()
{
    if (!m_process) return;

    // 收到进程输出 → 重置空闲超时（ffmpeg 还在工作）
    m_timer->start(120000);

    // 保存 stderr 缓冲区副本（onProcessFinished 也用它来获取错误信息）
    m_lastStderrBuffer = m_process->readAllStandardError();
    QString output = QString::fromLocal8Bit(m_lastStderrBuffer);

    // Parse progress from FFmpeg output
    // Look for "time=HH:MM:SS.ss" or "size=xxxkB"
    QRegularExpression timeRe(R"(time=(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    QRegularExpressionMatch match = timeRe.match(output);
    if (match.hasMatch() && m_totalDuration > 0) {
        qint64 hours = match.captured(1).toLongLong();
        qint64 minutes = match.captured(2).toLongLong();
        qint64 seconds = match.captured(3).toLongLong();
        qint64 centiseconds = match.captured(4).toLongLong();

        qint64 currentTime = (hours * 3600 + minutes * 60 + seconds) * 1000 + centiseconds * 10;
        double progressValue = qMin(1.0, static_cast<double>(currentTime) / m_totalDuration);
        emit progress(progressValue);
    }
}

void FFmpegService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_timer->stop();
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    QString error;

    if (!success) {
        // 原始 stderr（含完整技术细节，写入日志）
        QString rawError = QString::fromLocal8Bit(m_lastStderrBuffer);

        // 日志记录完整的原始错误
        PluginLogger::error(QString("FFmpeg %1 失败——原始错误: %2")
            .arg(m_isExtracting ? "音频提取" : "烧录字幕", rawError));

        // 界面展示简明的中文提示
        if (rawError.contains("Output file does not contain any stream")) {
            error = "视频中没有音频轨道，无法提取音频";
        } else if (rawError.contains("Invalid data found when processing input")) {
            error = "无法读取视频文件，文件可能已损坏或格式不支持";
        } else if (rawError.contains("No such file or directory")) {
            error = "视频文件不存在或路径错误";
        } else if (rawError.contains("Permission denied")) {
            error = "没有权限访问视频文件";
        } else if (rawError.contains("Invalid argument")) {
            error = "FFmpeg 参数错误，可能是视频编码格式不支持";
        } else {
            // 截取有意义的部分，去掉内存地址等杂乱信息
            QString cleaned = rawError;
            cleaned.replace(QRegularExpression("\[.*? @ [0-9a-fA-F]+\]"), "");
            cleaned.replace(QRegularExpression("\[.*? @ 0x[0-9a-fA-F]+\]"), "");
            cleaned = cleaned.trimmed();
            if (!cleaned.isEmpty()) {
                // 取最后一行有意义的信息（通常是核心错误描述）
                QStringList lines = cleaned.split('\n', Qt::SkipEmptyParts);
                for (const auto &l : lines) {
                    QString t = l.trimmed();
                    if (!t.startsWith('[') && !t.startsWith("Error ")) {
                        error = t;
                        break;
                    }
                }
                if (error.isEmpty())
                    error = lines.last().trimmed();
            }
            if (error.isEmpty())
                error = "处理视频时发生未知错误";
        }
    }

    m_process->deleteLater();
    m_process = nullptr;

    // GPU 加速失败 → 自动回退到软件编码重试
    if (!success && m_useHardwareAccel && !m_burnFallbackTried) {
        m_burnFallbackTried = true;
        m_useHardwareAccel = false;  // 禁用 GPU，下次用软件
        // GPU 回退：日志已在上方记录了原始 stderr，这里不再重复

        const auto &p = m_burnParams;
        // 重新调用 startBurnSubtitles（会重建 m_process，不会递归）
        startBurnSubtitles(p.ffmpegPath, p.videoPath, p.srtPath, p.outputPath,
                           p.fontSize, p.fontColor, p.borderColor, p.borderWidth);
        return;
    }

    emit progress(1.0);
    emit finished(success, m_outputPath, error);
}

void FFmpegService::onProcessTimeout()
{
    PluginLogger::error(QString("FFmpeg 进程超时，强制终止: %1").arg(m_outputPath));

    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
        m_process->deleteLater();
        m_process = nullptr;
    }

    emit progress(1.0);
    emit finished(false, m_outputPath, QString("%1 超时，进程已终止")
        .arg(m_isExtracting ? "音频提取" : "烧录字幕"));
}
