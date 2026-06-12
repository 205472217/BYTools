#include "FfmpegUtils.h"
#include "SubtitleUtils.h"
#include "Logger.h"

#include <QProcess>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>

// ── 辅助：编码器名称映射 ────────────────────────────────

namespace {

// 根据 GPU 厂商和输入编码选择对应的 GPU 编码器名称
QString gpuEncoderName(GpuVendor vendor, const QString &inputCodec)
{
    const bool hevc = (inputCodec == "hevc");
    switch (vendor) {
    case GpuVendor::CUDA:  return hevc ? "hevc_nvenc"  : "h264_nvenc";
    case GpuVendor::Intel: return hevc ? "hevc_qsv"    : "h264_qsv";
    case GpuVendor::AMD:   return hevc ? "hevc_amf"    : "h264_amf";
    default: return QString();
    }
}

} // namespace

// ── gpuVendorName ────────────────────────────────────────

QString gpuVendorName(GpuVendor vendor)
{
    switch (vendor) {
    case GpuVendor::CUDA:  return QStringLiteral("NVIDIA CUDA (NVENC)");
    case GpuVendor::AMD:   return QStringLiteral("AMD AMF");
    case GpuVendor::Intel: return QStringLiteral("Intel QSV");
    default:               return QStringLiteral("未检测到GPU加速");
    }
}

// ── detectGpuVendor ──────────────────────────────────────

GpuVendor detectGpuVendor(const QString &ffmpegPath)
{
    if (ffmpegPath.isEmpty())
        return GpuVendor::None;

    // 1. 查询 ffmpeg 编译了哪些 GPU 编码器
    QProcess proc;
    proc.start(ffmpegPath, {"-encoders"});
    if (!proc.waitForFinished(15000) || proc.exitCode() != 0)
        return GpuVendor::None;

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QString upper = output.toUpper();

    bool hasNVENC = upper.contains("NVENC");
    bool hasAMF   = upper.contains("AMF");
    bool hasQSV   = upper.contains("QSV");

    // 2. 辅助：用 ffmpeg 实际初始化硬件设备验证真伪
    auto probeDevice = [&](const QStringList &args) -> bool {
        QProcess p;
        p.start(ffmpegPath, args);
        p.waitForFinished(5000);
        return (p.exitCode() == 0);
    };

    // 检测优先级：NVIDIA → AMD → Intel
    // （同两个字幕插件的原有逻辑）

    // NVIDIA — 尝试初始化 CUDA 设备
    if (hasNVENC && probeDevice({"-init_hw_device", "cuda=probe", "-f", "null", "-"}))
        return GpuVendor::CUDA;

    // AMD AMF — 尝试用 h264_amf 编码一帧黑屏（确认驱动正常）
    if (hasAMF) {
        QProcess p;
        p.start(ffmpegPath, {"-f", "lavfi", "-i", "color=c=black:s=64x64:d=0.1",
                             "-c:v", "h264_amf", "-f", "null", "-"});
        p.waitForFinished(5000);
        if (p.exitCode() == 0)
            return GpuVendor::AMD;
    }

    // Intel QSV — 尝试初始化 QSV 设备
    if (hasQSV && probeDevice({"-init_hw_device", "qsv=probe", "-f", "null", "-"}))
        return GpuVendor::Intel;

    return GpuVendor::None;
}

// ── detectInputCodec ─────────────────────────────────────

QString detectInputCodec(const QString &ffmpegPath, const QString &videoPath)
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
    return "h264";
}

// ── buildSubtitleFilter ──────────────────────────────────

QString buildSubtitleFilter(const QString &subtitlePath,
                            const QString &fontName,
                            int fontSize,
                            const QString &fontColor,
                            const QString &borderColor,
                            int borderWidth,
                            int shadow)
{
    // 路径规范化：反斜杠 → 正斜杠
    QString subPath = QString(subtitlePath).replace("\\", "/");
    // 转义冒号 → \:，FFmpeg 滤镜参数以 : 为选项分隔符
    // 路径包含盘符 F: 或其它冒号时必须转义，否则会被截断
    subPath.replace(":", "\\:");

    QString assFontColor = htmlColorToAss(fontColor);
    QString assBorderColor = htmlColorToAss(borderColor);

    // 路径包单引号防止 FFmpeg < 4.2 无法处理 \: 转义时冒号截断
    return QString("subtitles=f='%1':force_style='FontName=%2,FontSize=%3,"
                   "PrimaryColour=%4,OutlineColour=%5,"
                   "BorderStyle=1,Outline=%6,Shadow=%7'")
        .arg(subPath,
             fontName,
             QString::number(fontSize),
             assFontColor,
             assBorderColor,
             QString::number(borderWidth),
             QString::number(shadow));
}

// ── buildGpuAccelArgs ────────────────────────────────────

QStringList buildGpuAccelArgs(GpuVendor vendor,
                              const QString &videoPath,
                              const QString &subtitleFilter,
                              const QString &outputPath,
                              const QString &inputCodec,
                              bool needFragMp4)
{
    QStringList args;
    if (vendor == GpuVendor::None)
        return args;

    QString encoder = gpuEncoderName(vendor, inputCodec);
    if (encoder.isEmpty())
        return args;

    switch (vendor) {
    case GpuVendor::CUDA:
        args << "-hwaccel" << "cuda"
             << "-hwaccel_output_format" << "cuda"
             << "-i" << videoPath
             << "-vf" << ("hwdownload,format=nv12," + subtitleFilter + ",hwupload_cuda")
             << "-c:v" << encoder
             << "-preset" << "p7"
             << "-cq" << "23"
             << "-c:a" << "copy"
             << "-y";
        if (needFragMp4)
            args << "-movflags" << "+frag_keyframe+separate_moof";
        args << outputPath;
        break;

    case GpuVendor::Intel:
        args << "-hwaccel" << "qsv"
             << "-hwaccel_output_format" << "qsv"
             << "-i" << videoPath
             << "-vf" << ("hwdownload=format=nv12," + subtitleFilter + ",hwupload=format=nv12")
             << "-c:v" << encoder
             << "-preset" << "veryfast"
             << "-global_quality" << "23"
             << "-c:a" << "copy"
             << "-y";
        if (needFragMp4)
            args << "-movflags" << "+frag_keyframe+separate_moof";
        args << outputPath;
        break;

    case GpuVendor::AMD:
        // AMD AMF — 不用 -hwaccel，因为 DXVA2/D3D11VA 与 subtitles CPU 滤镜冲突
        // （FFmpegMergeService 已验证此行为）
        args << "-i" << videoPath
             << "-vf" << subtitleFilter
             << "-c:v" << encoder
             << "-quality" << "quality"
             << "-c:a" << "copy"
             << "-y";
        if (needFragMp4)
            args << "-movflags" << "+frag_keyframe+separate_moof";
        args << outputPath;
        break;

    default:
        break;
    }
    return args;
}

// ── isFFmpegAvailable ────────────────────────────────────

bool isFFmpegAvailable(const QString &ffmpegPath, PluginLogger *logger)
{
    if (ffmpegPath.isEmpty()) return false;

    QString nativePath = QDir::toNativeSeparators(ffmpegPath);

    QProcess proc;
    proc.start(nativePath, {"-version"});

    if (!proc.waitForStarted(3000)) {
        if (logger) logger->warn(QString("FFmpeg 启动失败: %1, 错误: %2")
            .arg(nativePath, proc.errorString()));
        return false;
    }

    if (!proc.waitForFinished(5000)) {
        proc.kill();
        proc.waitForFinished(2000);
        if (logger) logger->warn(QString("FFmpeg 执行超时: %1").arg(nativePath));
        return false;
    }

    bool success = (proc.exitCode() == 0);
    if (!success && logger) {
        logger->warn(QString("FFmpeg 返回值非零: %1, exitCode=%2, stderr=%3")
            .arg(nativePath)
            .arg(proc.exitCode())
            .arg(QString::fromUtf8(proc.readAllStandardError()).trimmed()));
    }
    return success;
}

// ── ffmpegVersion ─────────────────────────────────────────

QString ffmpegVersion(const QString &ffmpegPath)
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

// ── getVideoDuration ──────────────────────────────────────

qint64 getVideoDuration(const QString &ffmpegPath, const QString &videoPath)
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

// ── extractFfmpegError ────────────────────────────────────

QString extractFfmpegError(const QString &rawStderr)
{
    QStringList lines = rawStderr.split('\n', Qt::SkipEmptyParts);

    // 从最后一行往前找，跳过进度行和 boilerplate
    for (int i = lines.size() - 1; i >= 0; --i) {
        QString t = lines[i].trimmed();
        if (t.isEmpty()) continue;

        // 跳过 ffmpeg 进度行
        if (t.contains("frame=") && t.contains("time=") && t.contains("bitrate="))
            continue;

        // 清理 [xxx @ 0x...] 内存地址噪声
        t.replace(QRegularExpression("\\[.*? @ 0x[0-9a-fA-F]+\\]\\s*"), "");
        t = t.trimmed();
        if (t.isEmpty()) continue;

        // 跳过 boilerplate
        if (t == "Error" || t.startsWith("Error ") || t == "Conversion failed!")
            continue;

        return t;
    }

    // Fallback: 最后一行非空行
    for (int i = lines.size() - 1; i >= 0; --i) {
        QString t = lines[i].trimmed();
        if (!t.isEmpty()) return t;
    }

    return QString();
}
