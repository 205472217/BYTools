#include "FFmpegMergeService.h"
#include "Config.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QCollator>
#include <algorithm>
#include <functional>

FFmpegMergeService::FFmpegMergeService(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_runner(new FfmpegRunner(this))
{
    m_runner->setLogger(m_logger);
    connect(m_runner, &FfmpegRunner::progress,
            this, &FFmpegMergeService::onRunnerProgress);
    connect(m_runner, &FfmpegRunner::finished,
            this, &FFmpegMergeService::onRunnerFinished);

    connect(&m_workerThread, &QThread::started,
            this, &FFmpegMergeService::doCollectFiles, Qt::DirectConnection);
    connect(&m_workerThread, &QThread::finished, this, [this]() {
        m_workerRunning = false;
    });
}

FFmpegMergeService::~FFmpegMergeService()
{
    cancel();
    m_workerThread.quit();
    m_workerThread.wait(5000);
}

void FFmpegMergeService::startMerge(const QString &ffmpegPath,
                                     const QString &videoDir,
                                     const QString &outputDir,
                                     bool recursive,
                                     bool useGpu)
{
    if (m_runner->isRunning()) {
        emit logMessage("✗ 已有合成任务正在执行，请等待完成");
        emit finished(false, "已有合成任务正在执行，请等待完成");
        return;
    }

    if (m_workerRunning) {
        emit logMessage("✗ 正在扫描中，请稍后");
        emit finished(false, "正在扫描中，请稍后");
        return;
    }

    m_ffmpegPath = ffmpegPath;
    m_videoDir = videoDir;
    m_outputDir = outputDir;
    m_recursive = recursive;
    m_currentIndex = -1;
    m_cancelled.storeRelaxed(0);
    m_stopTargetIndex = -1;
    m_successCount = 0;
    m_failCount = 0;
    m_pendingFiles.clear();
    m_checkUseGpu = useGpu;

    QDir outDir(outputDir);
    if (!outDir.exists()) {
        outDir.mkpath(".");
    }

    emit logMessage("步骤3：扫描视频文件...");
    m_logger->info(QString("扫描目录: %1 (递归=%2)").arg(videoDir).arg(recursive));

    m_workerRunning = true;
    m_workerThread.start();
}

void FFmpegMergeService::cancel()
{
    m_cancelled.storeRelaxed(1);
    if (m_workerRunning) {
        emit logMessage("  ⏹ 正在停止扫描...");
        m_workerThread.quit();
        if (!m_workerThread.wait(3000)) {
            m_workerThread.terminate();
            m_workerThread.wait(3000);
        }
    }
    if (m_runner->isRunning()) {
        emit logMessage("  ⏹ 正在等待 ffmpeg 优雅退出 (写入 moov atom)...");
    }
    m_runner->cancel();
    emit logMessage("  ⏹ 已取消");
}

void FFmpegMergeService::requestStopAfterCount(int count)
{
    m_stopTargetIndex = (count <= 0) ? -1 : (m_currentIndex + count);
}

void FFmpegMergeService::doCollectFiles()
{
    QStringList nameFilters;
    for (const QString &ext : videoExtensions())
        nameFilters << ("*" + ext);

    QSet<QString> seenPaths;
    m_pendingFiles.clear();

    if (m_recursive)
        emit logMessage("  正在递归查找视频文件...");
    else
        emit logMessage("  正在扫描视频文件...");

    std::function<void(const QString &)> collectDir;
    collectDir = [&](const QString &dirPath) {
        if (m_cancelled.loadRelaxed()) return;

        QDir dir(dirPath);
        QString relPath = QDir(m_videoDir).relativeFilePath(dirPath);
        if (relPath == ".")
            emit logMessage("  📁 " + QFileInfo(m_videoDir).fileName());
        else
            emit logMessage("  📁 " + relPath);

        QFileInfoList files = dir.entryInfoList(nameFilters, QDir::Files, QDir::NoSort);
        naturalSort(files);
        for (const QFileInfo &fi : files) {
            if (m_cancelled.loadRelaxed()) return;

            QString absPath = fi.absoluteFilePath();
            if (seenPaths.contains(absPath))
                continue;
            seenPaths.insert(absPath);

            QString basePath = fi.absolutePath() + "/" + fi.completeBaseName();
            QString subPath;
            for (const QString &ext : subtitleExtensions()) {
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
            vf.outputPath = m_outputDir + "/" + fi.fileName();

            if (QFileInfo::exists(vf.outputPath)) {
                int idx = 2;
                QString stem = fi.completeBaseName();
                QString ext = fi.suffix();
                while (QFileInfo::exists(m_outputDir + "/" + stem + "_副本" + QString::number(idx) + "." + ext))
                    idx++;
                vf.outputPath = m_outputDir + "/" + stem + "_副本" + QString::number(idx) + "." + ext;
            }

            m_pendingFiles.append(vf);
            emit logMessage("    + " + fi.fileName());
        }

        if (!m_recursive) return;

        QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
        naturalSort(subdirs);
        for (const QFileInfo &subdir : subdirs) {
            if (m_cancelled.loadRelaxed()) return;
            collectDir(subdir.absoluteFilePath());
        }
    };

    collectDir(m_videoDir);

    m_workerThread.quit();
    QMetaObject::invokeMethod(this, "onCollectFinished", Qt::QueuedConnection);
}

void FFmpegMergeService::onCollectFinished()
{
    if (m_cancelled.loadRelaxed()) {
        emit logMessage("  ⏹ 扫描已取消");
        emit finished(false, "已取消");
        return;
    }

    // 去重：输出路径相同的任务只保留第一个，避免多次合成相同视频
    {
        QSet<QString> seenOutputs;
        QList<VideoFile> deduped;
        int dupCount = 0;
        for (const auto &vf : m_pendingFiles) {
            if (seenOutputs.contains(vf.outputPath)) {
                emit logMessage("    ⏭ 跳过重复输出: " + QFileInfo(vf.path).fileName());
                dupCount++;
                continue;
            }
            seenOutputs.insert(vf.outputPath);
            deduped.append(vf);
        }
        if (dupCount > 0) {
            emit logMessage(QString("  ⏭ 去重完成，移除了 %1 个重复视频").arg(dupCount));
        }
        m_pendingFiles = deduped;
    }

    if (m_pendingFiles.isEmpty()) {
        QString err = "✗ 所选文件夹中未找到带字幕的视频文件\n"
                      "  请检查：\n"
                      "  · 视频文件格式是否为 mp4/mkv/avi/mov 等常见格式\n"
                      "  · 视频文件旁边是否有同名的 .srt/.ass 字幕文件\n"
                      "  · 勾选「递归扫描」可搜索子文件夹中的视频";
        emit logMessage(err);
        m_logger->error(QString("未找到带字幕的视频文件 (目录=%1, 递归=%2)")
            .arg(m_videoDir).arg(m_recursive));
        emit finished(false, "所选文件夹中未找到带字幕的视频文件（视频需与同名字幕放在一起）");
        return;
    }

    emit logMessage(QString("找到 %1 个带字幕的视频文件，开始合成...").arg(m_pendingFiles.size()));
    m_logger->info(QString("待处理文件数=%1, 输出目录=%2, 递归=%3, GPU=%4")
        .arg(m_pendingFiles.size()).arg(m_outputDir).arg(m_recursive)
        .arg(m_checkUseGpu ? "启用(每次处理前检测)" : "禁用"));

    processNextFile();
}

void FFmpegMergeService::processNextFile()
{
    m_currentIndex++;
    bool stopHit = (m_stopTargetIndex >= 0 && m_currentIndex >= m_stopTargetIndex);
    if (m_cancelled || stopHit || m_currentIndex >= m_pendingFiles.size()) {
        QString msg = QString("合成完成: 成功 %1, 失败 %2").arg(m_successCount).arg(m_failCount);
        if (stopHit && m_currentIndex < m_pendingFiles.size()) {
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

    // 填配置，调用黑盒 runner（GPU 由 FfmpegRunner 内部自动检测）
    FfmpegRunner::BurnConfig cfg;
    cfg.ffmpegPath    = m_ffmpegPath;
    cfg.videoPath     = vf.path;
    cfg.subtitlePath  = vf.subtitlePath;
    cfg.outputPath    = vf.outputPath;
    cfg.useGpu        = m_checkUseGpu;
    cfg.gpuVendor     = GpuVendor::None; // 每次重新检测，避免跨文件 GPU 状态变化

    m_runner->burnSubtitles(cfg);

    emit progress(double(m_currentIndex) / m_pendingFiles.size());
    emit currentFileProgress(0.0);
}

// ── Runner slots ──────────────────────────────────────────

void FFmpegMergeService::onRunnerProgress(double value)
{
    // 原始 value 是当前单个文件的 ffmpeg 进度（基于视频时长）
    emit currentFileProgress(value);

    // 同时保留整体进度
    double overall = (m_currentIndex + value) / m_pendingFiles.size();
    emit progress(overall);
}

void FFmpegMergeService::onRunnerFinished(bool success, const QString &outputPath, const QString &error)
{
    if (m_cancelled) {
        emit logMessage("  ⏹ 已取消");
        processNextFile();
        return;
    }

    const auto &vf = m_pendingFiles[m_currentIndex];
    QFileInfo fi(vf.path);

    if (success) {
        emit logMessage("  ✓ 合成完成: " + fi.fileName());
        m_successCount++;
    } else {
        QString errMsg = error.isEmpty() ? extractFfmpegError(QString()) : error;
        if (errMsg.isEmpty()) errMsg = "未知错误";
        emit logMessage("  ✗ 合成失败: " + fi.fileName() + " - " + errMsg);
        m_logger->error(QString("合成失败 [%1]: %2").arg(fi.fileName(), error));
        m_failCount++;
    }

    emit currentFileProgress(1.0);
    emit progress(double(m_currentIndex + 1) / m_pendingFiles.size());
    processNextFile();
}
