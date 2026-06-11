#include "FFmpegMergeService.h"
#include "PluginLogger.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QCollator>
#include <algorithm>
#include <functional>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

FFmpegMergeService::FFmpegMergeService(QObject *parent)
    : QObject(parent)
{
}

bool FFmpegMergeService::isFFmpegAvailable(const QString &ffmpegPath)
{
    if (ffmpegPath.isEmpty()) return false;
    QProcess proc;
    proc.start(ffmpegPath, {"-version"});
    proc.waitForFinished(5000);
    return proc.exitCode() == 0;
}

FFmpegMergeService::GpuVendor FFmpegMergeService::detectGpuVendor(const QString &ffmpegPath)
{
    QProcess proc;
    proc.start(ffmpegPath, {"-encoders"});
    if (!proc.waitForFinished(10000) || proc.exitCode() != 0) {
        return GpuVendor::None;
    }
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QString upper = output.toUpper();

    bool hasNVENC = upper.contains("NVENC");
    bool hasAMF   = upper.contains("AMF");
    bool hasQSV   = upper.contains("QSV");

    // 辅助：用 ffmpeg 实际初始化硬件设备来验证真伪（vs 只看编译列表）
    auto probeDevice = [&](const QStringList &args) -> bool {
        QProcess p;
        p.start(ffmpegPath, args);
        p.waitForFinished(5000);
        return (p.exitCode() == 0);
    };

    // 检测优先级：NVIDIA → AMD → Intel → CPU（同 videosubtitle 插件）

    // 1. NVIDIA — 尝试初始化 CUDA 设备（检查 nvcuda.dll 能否加载）
    if (hasNVENC && probeDevice({"-init_hw_device", "cuda=probe", "-f", "null", "-"}))
        return GpuVendor::CUDA;

    // 2. AMD AMF — 尝试用 h264_amf 编码一帧黑屏（确认驱动正常）
    if (hasAMF) {
        QProcess p;
        p.start(ffmpegPath, {"-f", "lavfi", "-i", "color=c=black:s=64x64:d=0.1",
                             "-c:v", "h264_amf", "-f", "null", "-"});
        p.waitForFinished(5000);
        if (p.exitCode() == 0)
            return GpuVendor::AMD;
    }

    // 3. Intel QSV — 尝试初始化 QSV 设备
    if (hasQSV && probeDevice({"-init_hw_device", "qsv=probe", "-f", "null", "-"}))
        return GpuVendor::Intel;

    return GpuVendor::None;
}

QString FFmpegMergeService::gpuVendorToString(GpuVendor vendor)
{
    switch (vendor) {
        case GpuVendor::CUDA:   return QStringLiteral("NVIDIA CUDA (NVENC)");
        case GpuVendor::AMD:    return QStringLiteral("AMD AMF");
        case GpuVendor::Intel:  return QStringLiteral("Intel QSV");
        default:                return QStringLiteral("未检测到GPU加速");
    }
}

QString FFmpegMergeService::detectInputCodec(const QString &ffmpegPath, const QString &videoPath)
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

QStringList FFmpegMergeService::buildGpuAccelArgs(const QString &videoPath,
                                                    const QString &subtitlePath,
                                                    const QString &outputPath)
{
    QStringList args;

    // Detect input codec to pick the right GPU encoder variant
    QString codec = detectInputCodec(m_ffmpegPath, videoPath);

    // Build subtitle style filter (always CPU-side)
    // 必须用 \\: 双反斜杠转义冒号。ffmpeg 4.x 的 graph parser (av_get_token buf,;,)
    // 先吃一层 \ → 剩 \:, 然后 av_set_options_string (av_get_token buf,:)
    // 再吃一层 \ → 剩 :。单反斜杠 \: 在第一层就被消耗了，暴露的原始 : 会切碎路径。
    QString escapedSub = QString(subtitlePath).replace("\\", "/").replace(":", "\\\\:");
    QString styleFilter = QString("subtitles=f=%1:force_style='FontName=Microsoft YaHei,FontSize=20,"
                                  "PrimaryColour=&H00FFFFFF,OutlineColour=&H00000000,"
                                  "BorderStyle=1,Outline=1,Shadow=0'").arg(escapedSub);

    // Pick GPU encoder based on vendor + input codec
    auto gpuEncoder = [&](const QString &nvidia, const QString &intel, const QString &amd) -> QString {
        if (codec == "hevc") {
            switch (m_gpuVendor) {
            case GpuVendor::CUDA:  return "hevc_" + nvidia;
            case GpuVendor::Intel: return "hevc_" + intel;
            case GpuVendor::AMD:   return "hevc_" + amd;
            default: break;
            }
        }
        switch (m_gpuVendor) {
        case GpuVendor::CUDA:  return "h264_" + nvidia;
        case GpuVendor::Intel: return "h264_" + intel;
        case GpuVendor::AMD:   return "h264_" + amd;
        default: break;
        }
        return QString();
    };

    switch (m_gpuVendor) {
    case GpuVendor::CUDA: {
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
    case GpuVendor::Intel: {
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
    case GpuVendor::AMD: {
        // AMD AMF — 不用 -hwaccel d3d11va，因为它与 subtitles CPU 滤镜冲突
        // AMF 编码器自身就是 GPU 加速的，解码用 CPU 就足够
        QString encoder = gpuEncoder("nvenc", "qsv", "amf");
        args << "-i" << videoPath
             << "-vf" << styleFilter
             << "-c:v" << encoder
             << "-quality" << "quality"
             << "-c:a" << "copy"
             << "-y"
             << outputPath;
        break;
    }
    default:
        break;
    }
    return args;
}

void FFmpegMergeService::buildFfmpegArgs(const QString &videoPath,
                                          const QString &subtitlePath,
                                          const QString &outputPath,
                                          QStringList &args)
{
    args << "-i" << videoPath;

    // Subtitle burn filter (CPU-based)
    QString escapedSub = QString(subtitlePath).replace("\\", "/").replace(":", "\\\\:");
    QString vfFilter = QString("subtitles=f=%1:force_style='FontName=Microsoft YaHei,FontSize=20,"
                               "PrimaryColour=&H00FFFFFF,OutlineColour=&H00000000,"
                               "BorderStyle=1,Outline=1,Shadow=0'").arg(escapedSub);
    args << "-vf" << vfFilter;

    // Copy audio stream
    args << "-c:a" << "copy"
         << "-y"
         << outputPath;
}

void FFmpegMergeService::startMerge(const QString &ffmpegPath,
                                     const QString &videoDir,
                                     const QString &outputDir,
                                     bool recursive,
                                     bool useGpu)
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        emit logMessage("✗ 已有合成任务正在执行，请等待完成");
        emit finished(false, "已有合成任务正在执行，请等待完成");
        return;
    }

    m_ffmpegPath = ffmpegPath;
    m_outputDir = outputDir;
    m_currentIndex = -1;
    m_cancelled = false;
    m_stopAfterCurrent = false;
    m_successCount = 0;
    m_failCount = 0;
    m_lastStderrBuffer.clear();
    m_pendingFiles.clear();
    m_useGpu = useGpu;
    m_burnFallbackTried = false;
    m_totalDuration = 0;

    // GPU auto-detection
    m_gpuVendor = GpuVendor::None;
    if (useGpu) {
        m_gpuVendor = detectGpuVendor(ffmpegPath);
        if (m_gpuVendor != GpuVendor::None) {
            emit logMessage(QString("✓ 检测到GPU加速: %1").arg(gpuVendorToString(m_gpuVendor)));
        } else {
            emit logMessage("✗ 未检测到可用GPU编码器，将使用CPU编码");
        }
    }

    QDir outDir(outputDir);
    if (!outDir.exists()) {
        outDir.mkpath(".");
    }

    emit logMessage("步骤3：扫描视频文件...");
    PluginLogger::info(QString("扫描目录: %1 (递归=%2)").arg(videoDir).arg(recursive));

    // 构建视频扩展名过滤器列表
    QStringList nameFilters;
    for (const QString &ext : m_videoExts)
        nameFilters << ("*" + ext);

    // 递归扫描，按 父目录→子目录(排序)→文件(排序) 的顺序
    // 同资源管理器打开文件夹看到的顺序一致
    QSet<QString> seenPaths;  // 去重保护，防止符号链接等导致同一文件被处理两次
    std::function<void(const QString &)> collectDir;
    collectDir = [&](const QString &dirPath) {
        QDir dir(dirPath);
        QString relPath = QDir(videoDir).relativeFilePath(dirPath);
        if (relPath == ".")
            emit logMessage("  📁 " + QFileInfo(videoDir).fileName());
        else
            emit logMessage("  📁 " + relPath);

        // 1. 当前目录的视频文件，按自然名称排序（同 Windows 资源管理器）
        QFileInfoList files = dir.entryInfoList(nameFilters, QDir::Files, QDir::NoSort);
        { QCollator c; c.setNumericMode(true);
        std::sort(files.begin(), files.end(), [&](const QFileInfo &a, const QFileInfo &b) {
            return c.compare(a.fileName(), b.fileName()) < 0; }); }
        for (const QFileInfo &fi : files) {
            // 去重
            QString absPath = fi.absoluteFilePath();
            if (seenPaths.contains(absPath))
                continue;
            seenPaths.insert(absPath);

            // 查找同名字幕文件
            QString basePath = fi.absolutePath() + "/" + fi.completeBaseName();
            QString subPath;
            for (const QString &ext : m_subtitleExts) {
                QString candidate = basePath + ext;
                if (QFileInfo::exists(candidate)) {
                    subPath = candidate;
                    break;
                }
            }
            if (subPath.isEmpty()) continue;

            VideoFile vf;
            vf.path = absPath;
            vf.subtitlePath = subPath;
            vf.outputPath = outputDir + "/" + fi.fileName();

            // 输出目录中文件名冲突处理
            if (QFileInfo::exists(vf.outputPath)) {
                int idx = 2;
                QString stem = fi.completeBaseName();
                QString ext = fi.suffix();
                while (QFileInfo::exists(outputDir + "/" + stem + "_副本" + QString::number(idx) + "." + ext))
                    idx++;
                vf.outputPath = outputDir + "/" + stem + "_副本" + QString::number(idx) + "." + ext;
            }

            m_pendingFiles.append(vf);
            emit logMessage("    + " + fi.fileName());
        }

        if (!recursive) return;

        // 2. 子目录，按自然名称排序（递归）
        QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
        { QCollator c; c.setNumericMode(true);
        std::sort(subdirs.begin(), subdirs.end(), [&](const QFileInfo &a, const QFileInfo &b) {
            return c.compare(a.fileName(), b.fileName()) < 0; }); }
        for (const QFileInfo &subdir : subdirs) {
            collectDir(subdir.absoluteFilePath());
        }
    };

    collectDir(videoDir);

    if (m_pendingFiles.isEmpty()) {
        QString err = "✗ 所选文件夹中未找到带字幕的视频文件\n"
                      "  请检查：\n"
                      "  · 视频文件格式是否为 mp4/mkv/avi/mov 等常见格式\n"
                      "  · 视频文件旁边是否有同名的 .srt/.ass 字幕文件\n"
                      "  · 勾选「递归扫描」可搜索子文件夹中的视频";
        emit logMessage(err);
        PluginLogger::error(QString("未找到带字幕的视频文件 (目录=%1, 递归=%2)")
            .arg(videoDir).arg(recursive));
        emit finished(false, "所选文件夹中未找到带字幕的视频文件（视频需与同名字幕放在一起）");
        return;
    }

    emit logMessage(QString("找到 %1 个带字幕的视频文件，开始合成...").arg(m_pendingFiles.size()));
    PluginLogger::info(QString("待处理文件数=%1, 输出目录=%2, 递归=%3, GPU=%4")
        .arg(m_pendingFiles.size()).arg(m_outputDir).arg(recursive)
        .arg(gpuVendorToString(m_gpuVendor)));

    if (!m_process) {
        m_process = new QProcess(this);
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        connect(m_process, &QProcess::readyReadStandardError,
                this, &FFmpegMergeService::onProcessReadyRead);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &FFmpegMergeService::onProcessFinished);
        connect(m_timer, &QTimer::timeout,
                this, &FFmpegMergeService::onProcessTimeout);
    }

    processNextFile();
}

void FFmpegMergeService::cancel()
{
    m_cancelled = true;
    if (m_timer) m_timer->stop();

    // 清除 ffmpeg 写到一半的残缺输出文件
    if (m_currentIndex >= 0 && m_currentIndex < m_pendingFiles.size()) {
        QString partialPath = m_pendingFiles[m_currentIndex].outputPath;
        if (QFileInfo::exists(partialPath)) {
            QFile::remove(partialPath);
            emit logMessage("  [清理] 已删除半合成文件: " + QFileInfo(partialPath).fileName());
        }
    }

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void FFmpegMergeService::requestStopAfterCurrent()
{
    m_stopAfterCurrent = true;
    emit logMessage("  ⏹ 已预约停止，当前视频处理完后将不再继续");
}

void FFmpegMergeService::processNextFile()
{
    m_currentIndex++;
    if (m_cancelled || m_stopAfterCurrent || m_currentIndex >= m_pendingFiles.size()) {
        QString msg = QString("合成完成: 成功 %1, 失败 %2").arg(m_successCount).arg(m_failCount);
        if (m_stopAfterCurrent && m_currentIndex < m_pendingFiles.size()) {
            msg += QString(" (已跳过剩余 %1 个)").arg(m_pendingFiles.size() - m_currentIndex);
        }
        emit logMessage(msg);
        emit finished(true, msg);
        return;
    }

    const auto &vf = m_pendingFiles[m_currentIndex];
    QFileInfo fi(vf.path);

    emit currentFileChanged(vf.path);
    emit logMessage(QString("[%1/%2] 合成: %3")
        .arg(m_currentIndex + 1).arg(m_pendingFiles.size()).arg(fi.fileName()));

    // Build ffmpeg command
    QStringList args;
    if (m_useGpu && m_gpuVendor != GpuVendor::None) {
        args = buildGpuAccelArgs(vf.path, vf.subtitlePath, vf.outputPath);
        if (args.isEmpty()) {
            // GPU arg building failed for some reason, fall through to software
            emit logMessage("  ⚠ GPU参数构建失败，回退到CPU编码");
        } else {
            PluginLogger::info("使用GPU加速合成: " + gpuVendorToString(m_gpuVendor));
        }
    }

    if (args.isEmpty()) {
        // Software encoding (CPU)
        buildFfmpegArgs(vf.path, vf.subtitlePath, vf.outputPath, args);
    }

    // Cache params for GPU fallback retry
    m_burnParams = {m_ffmpegPath, vf.path, vf.subtitlePath, vf.outputPath};
    m_burnFallbackTried = false;

    // Get total duration for progress
    m_totalDuration = 0;
    QProcess durProc;
    durProc.start(m_ffmpegPath, {"-i", vf.path});
    if (durProc.waitForFinished(10000)) {
        QString durOutput = QString::fromUtf8(durProc.readAllStandardError());
        QRegularExpression durRe(R"(Duration:\s*(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
        QRegularExpressionMatch dm = durRe.match(durOutput);
        if (dm.hasMatch()) {
            qint64 h = dm.captured(1).toLongLong();
            qint64 m = dm.captured(2).toLongLong();
            qint64 s = dm.captured(3).toLongLong();
            qint64 cs = dm.captured(4).toLongLong();
            m_totalDuration = (h * 3600 + m * 60 + s) * 1000 + cs * 10;
        }
    }

    PluginLogger::info("ffmpeg args: " + args.join(" "));

    m_lastStderrBuffer.clear();
    m_process->start(m_ffmpegPath, args);

    // Idle timeout: as long as ffmpeg keeps outputting progress, the timer gets reset
    m_timer->start(120000);

    emit progress(double(m_currentIndex) / m_pendingFiles.size());
}

void FFmpegMergeService::onProcessReadyRead()
{
    if (!m_process) return;

    // Reset idle timeout - ffmpeg is still running
    m_timer->start(120000);

    // Accumulate stderr for error reporting in onProcessFinished (limit to 64KB)
    QByteArray chunk = m_process->readAllStandardError();
    m_lastStderrBuffer += chunk;
    if (m_lastStderrBuffer.size() > 65536)
        m_lastStderrBuffer = m_lastStderrBuffer.right(65536);
    QString output = QString::fromUtf8(chunk);

    // Extract time progress for display
    static QRegularExpression timeRe(R"(time=(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    QRegularExpressionMatch m = timeRe.match(output);
    if (m.hasMatch() && m_totalDuration > 0) {
        qint64 h = m.captured(1).toLongLong();
        qint64 min = m.captured(2).toLongLong();
        qint64 s = m.captured(3).toLongLong();
        qint64 currentMs = (h * 3600 + min * 60 + s) * 1000;
        double p = qMin(1.0, static_cast<double>(currentMs) / m_totalDuration);
        emit progress(p);
    }

#ifdef Q_OS_WIN
    // Non-blocking Q key check (fires with each ffmpeg progress tick ≈ 1s)
    if (!m_stopAfterCurrent && !m_cancelled && (GetAsyncKeyState('Q') & 0x8000)) {
        m_stopAfterCurrent = true;
        emit logMessage("  ⏹ 已预约停止，当前视频处理完后将不再继续（按Q停止）");
    }
#endif
}

void FFmpegMergeService::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_timer->stop();

    if (m_cancelled) {
        emit logMessage("  ⏹ 已取消");
        processNextFile();
        return;
    }

    const auto &vf = m_pendingFiles[m_currentIndex];
    QFileInfo fi(vf.path);

    bool success = (status == QProcess::NormalExit && exitCode == 0);
    QString errorDetail;

    if (success) {
        emit logMessage("  ✓ 合成完成: " + fi.fileName());
        m_successCount++;
    } else {
        // Get full stderr for error reporting
        QString rawError = QString::fromUtf8(m_lastStderrBuffer);

        // Log full error to file
        PluginLogger::error(QString("合成失败 [%1]: %2")
            .arg(fi.fileName(), rawError));

        // Extract meaningful error for UI: take last non-empty, non-progress lines
        QStringList lines = rawError.split('\n', Qt::SkipEmptyParts);
        for (int i = lines.size() - 1; i >= 0; --i) {
            QString t = lines[i].trimmed();
            if (t.isEmpty()) continue;
            // skip ffmpeg progress lines (contain "frame=" or "time=")
            if (t.contains("frame=") && t.contains("time=") && t.contains("bitrate="))
                continue;
            // clean up [xxx @ 0x...] noise
            t.replace(QRegularExpression("\\[.*? @ 0x[0-9a-fA-F]+\\]\\s*"), "");
            t = t.trimmed();
            if (t.isEmpty()) continue;
            // skip lines that are just "Error" boilerplate
            if (t == "Error" || t.startsWith("Error ") || t == "Conversion failed!")
                continue;
            errorDetail = t;
            break;
        }
        if (errorDetail.isEmpty()) {
            // fallback: last non-empty line
            for (int i = lines.size() - 1; i >= 0; --i) {
                QString t = lines[i].trimmed();
                if (!t.isEmpty()) { errorDetail = t; break; }
            }
        }
        if (errorDetail.isEmpty())
            errorDetail = QString("退出码=%1").arg(exitCode);

        emit logMessage("  ✗ 合成失败: " + fi.fileName() + " - " + errorDetail);

        // GPU failure → automatic fallback to software encoding (only this video)
        if (m_useGpu && !m_burnFallbackTried && m_gpuVendor != GpuVendor::None) {
            m_burnFallbackTried = true;
            emit logMessage("  ⚠ GPU加速失败，正在回退到CPU编码（后续视频仍会尝试GPU）...");

            // Re-run current file with software encoding
            // Note: m_gpuVendor 保持不变，后续视频仍会尝试 GPU 加速
            const auto &p = m_burnParams;
            m_ffmpegPath = p.ffmpegPath;
            QStringList swArgs;
            buildFfmpegArgs(p.videoPath, p.subtitlePath, p.outputPath, swArgs);
            PluginLogger::info("GPU回退CPU ffmpeg args: " + swArgs.join(" "));

            m_lastStderrBuffer.clear();
            m_process->start(m_ffmpegPath, swArgs);
            m_timer->start(120000);
            return;
        }

        m_failCount++;
    }

    emit progress(double(m_currentIndex + 1) / m_pendingFiles.size());
    processNextFile();
}

void FFmpegMergeService::onProcessTimeout()
{
    PluginLogger::error("FFmpeg 进程超时，强制终止");

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }

    emit logMessage("  ⏰ 合成超时，进程已终止");
    m_failCount++;

    emit progress(double(m_currentIndex + 1) / m_pendingFiles.size());
    processNextFile();
}
