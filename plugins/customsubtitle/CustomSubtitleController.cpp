#include "CustomSubtitleController.h"
#include "SubtitleMatcher.h"
#include "FFmpegMergeService.h"
#include "FfmpegUtils.h"
#include "VideoReplaceService.h"
#include "Config.h"
#include "Logger.h"
#include "SettingsHelper.h"
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

CustomSubtitleController::CustomSubtitleController(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_matcher(new SubtitleMatcher(m_logger, this))
    , m_mergeService(new FFmpegMergeService(m_logger, this))
    , m_replaceService(new VideoReplaceService(m_logger, this))
{
    // Connect matcher signals
    connect(m_matcher, &SubtitleMatcher::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_matcher, &SubtitleMatcher::progress,
            this, &CustomSubtitleController::setProgress);
    connect(m_matcher, &SubtitleMatcher::progress,
            this, [this](double value) {
        if (m_totalCount > 0)
            setProcessedCount(qMin(m_totalCount, qRound(value * m_totalCount)));
    });
    connect(m_matcher, &SubtitleMatcher::finished,
            this, &CustomSubtitleController::onMatchFinished);

    // Connect merge service signals
    connect(m_mergeService, &FFmpegMergeService::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_mergeService, &FFmpegMergeService::progress,
            this, &CustomSubtitleController::setProgress);
    connect(m_mergeService, &FFmpegMergeService::finished,
            this, &CustomSubtitleController::onMergeFinished);
    connect(m_mergeService, &FFmpegMergeService::currentFileChanged,
            this, [this](const QString &path) {
        setCurrentFile(path);
    });
    connect(m_mergeService, &FFmpegMergeService::currentFileProgress,
            this, &CustomSubtitleController::setCurrentFileProgress);
    // Step 3 显示进度条+百分比，不追踪 N/M 计数

    // Connect replace service signals
    connect(m_replaceService, &VideoReplaceService::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_replaceService, &VideoReplaceService::progress,
            this, &CustomSubtitleController::setProgress);
    connect(m_replaceService, &VideoReplaceService::finished,
            this, &CustomSubtitleController::onReplaceFinished);
    connect(m_replaceService, &VideoReplaceService::scanFinished,
            this, [this](int matched) {
        setTotalCount(matched);
    });
    connect(m_replaceService, &VideoReplaceService::progress,
            this, [this](double value) {
        if (m_totalCount > 0)
            setProcessedCount(qMin(m_totalCount, qRound(value * m_totalCount)));
    });
    connect(m_replaceService, &VideoReplaceService::currentFileChanged,
            this, [this](const QString &path) {
        setCurrentFile(path);
    });

    // Load persisted paths
    QSettings &s = pluginGroupSettings("customsubtitle");
    s.sync();
    m_subtitleDownloadPath = s.value("customSubtitleDownloadPath").toString();
    m_videoSourcePath = s.value("customVideoSourcePath").toString();
    m_mergedOutputPath = s.value("customMergedOutputPath").toString();
    m_ffmpegPath = s.value("customFfmpegPath").toString();
    m_recursive = s.value("customRecursive", false).toBool();
    m_gpuAccel = s.value("customGpuAccel", false).toBool();
    m_fragmentedMp4 = s.value("customFragmentedMp4", true).toBool();
}

CustomSubtitleController::~CustomSubtitleController() = default;

// ── Getters ──
QString CustomSubtitleController::subtitleDownloadPath() const { return m_subtitleDownloadPath; }
QString CustomSubtitleController::videoSourcePath() const { return m_videoSourcePath; }
bool CustomSubtitleController::recursive() const { return m_recursive; }
QString CustomSubtitleController::mergedOutputPath() const { return m_mergedOutputPath; }
bool CustomSubtitleController::gpuAccel() const { return m_gpuAccel; }
bool CustomSubtitleController::fragmentedMp4() const { return m_fragmentedMp4; }
bool CustomSubtitleController::removeSrtAfterReplace() const { return m_removeSrtAfterReplace; }
bool CustomSubtitleController::backupOriginal() const { return m_backupOriginal; }
QString CustomSubtitleController::statusMessage() const { return m_statusMessage; }
double CustomSubtitleController::progress() const { return m_progress; }
double CustomSubtitleController::currentFileProgress() const { return m_currentFileProgress; }
bool CustomSubtitleController::isProcessing() const { return m_isProcessing; }
QString CustomSubtitleController::currentStep() const { return m_currentStep; }
int CustomSubtitleController::processedCount() const { return m_processedCount; }
int CustomSubtitleController::totalCount() const { return m_totalCount; }
QString CustomSubtitleController::currentFile() const { return m_currentFile; }

// ── Setters ──
void CustomSubtitleController::setSubtitleDownloadPath(const QString &path)
{
    if (m_subtitleDownloadPath != path) {
        m_subtitleDownloadPath = path;
        pluginGroupSettings("customsubtitle").setValue("customSubtitleDownloadPath", path);
        pluginGroupSettings("customsubtitle").sync();
        emit subtitleDownloadPathChanged();
    }
}

void CustomSubtitleController::setVideoSourcePath(const QString &path)
{
    if (m_videoSourcePath != path) {
        m_videoSourcePath = path;
        pluginGroupSettings("customsubtitle").setValue("customVideoSourcePath", path);
        pluginGroupSettings("customsubtitle").sync();
        emit videoSourcePathChanged();
    }
}

void CustomSubtitleController::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        pluginGroupSettings("customsubtitle").setValue("customRecursive", recursive);
        pluginGroupSettings("customsubtitle").sync();
        emit recursiveChanged();
    }
}

void CustomSubtitleController::setMergedOutputPath(const QString &path)
{
    if (m_mergedOutputPath != path) {
        m_mergedOutputPath = path;
        pluginGroupSettings("customsubtitle").setValue("customMergedOutputPath", path);
        pluginGroupSettings("customsubtitle").sync();
        emit mergedOutputPathChanged();
    }
}

void CustomSubtitleController::setFfmpegPath(const QString &path)
{
    if (m_ffmpegPath != path) {
        m_ffmpegPath = path;
        pluginGroupSettings("customsubtitle").setValue("customFfmpegPath", path);
        pluginGroupSettings("customsubtitle").sync();
        emit ffmpegPathChanged();
    }
}

void CustomSubtitleController::setGpuAccel(bool enable)
{
    if (m_gpuAccel != enable) {
        m_gpuAccel = enable;
        pluginGroupSettings("customsubtitle").setValue("customGpuAccel", enable);
        pluginGroupSettings("customsubtitle").sync();
        emit gpuAccelChanged();
    }
}

void CustomSubtitleController::setFragmentedMp4(bool enable)
{
    if (m_fragmentedMp4 != enable) {
        m_fragmentedMp4 = enable;
        pluginGroupSettings("customsubtitle").setValue("customFragmentedMp4", enable);
        pluginGroupSettings("customsubtitle").sync();
        emit fragmentedMp4Changed();
    }
}

void CustomSubtitleController::setRemoveSrtAfterReplace(bool remove)
{
    if (m_removeSrtAfterReplace != remove) {
        m_removeSrtAfterReplace = remove;
        emit removeSrtAfterReplaceChanged();
    }
}

void CustomSubtitleController::setBackupOriginal(bool backup)
{
    if (m_backupOriginal != backup) {
        m_backupOriginal = backup;
        emit backupOriginalChanged();
    }
}

// ── Actions ──

void CustomSubtitleController::matchAndMoveSubtitles()
{
    if (m_isProcessing) return;

    if (m_subtitleDownloadPath.isEmpty() || m_videoSourcePath.isEmpty()) {
        setStatusMessage("请先配置字幕下载路径和原视频路径");
        return;
    }

    setCurrentStep("匹配并移动字幕");
    setIsProcessing(true);
    setProgress(0.0);
    emit logMessage("========== 步骤2：匹配并移动字幕 ==========");

    m_logger->info(QString("匹配字幕: %1 → %2 (递归=%3)")
        .arg(m_subtitleDownloadPath, m_videoSourcePath).arg(m_recursive));

    // Run matching (blocking in current thread for simplicity; for large dirs use QThread)
    // 注意：matchSubtitles 内部出错会 emit finished() → 同步触发 onMatchFinished → 设 isProcessing=false
    QList<SubtitleMatcher::MatchResult> results = m_matcher->matchSubtitles(
        m_subtitleDownloadPath, m_videoSourcePath, m_recursive, {});

    // matchSubtitles 的 finished 信号已经处理了错误，不再重复处理
    if (!m_isProcessing)
        return;

    if (results.isEmpty()) {
        onMatchFinished(false, "未找到匹配结果");
        return;
    }

    setTotalCount(results.size());
    setProcessedCount(0);

    // Execute rename
    emit logMessage("--- 重命名 ---");
    int renamed = m_matcher->executeRename(results);
    emit logMessage(QString("重命名完成: %1 个").arg(renamed));

    if (renamed == 0) {
        setIsProcessing(false);
        setCurrentStep("");
        onMatchFinished(false, "重命名失败");
        return;
    }

    // Execute move
    emit logMessage("--- 移动 ---");
    int moved = m_matcher->executeMove(results);
    emit logMessage(QString("移动完成: %1 个").arg(moved));

    setProgress(1.0);
    setStatusMessage(QString("步骤2完成: 重命名 %1, 移动 %2").arg(renamed).arg(moved));
    setIsProcessing(false);
    setCurrentStep("");
    onMatchFinished(true, "");
}

void CustomSubtitleController::mergeSubtitleToVideo()
{
    if (m_isProcessing) return;

    QString ffmpeg = ffmpegPath();
    if (ffmpeg.isEmpty() || !isFFmpegAvailable(ffmpeg, m_logger)) {
        setStatusMessage("FFmpeg 不可用，请在自定义字幕设置中配置 FFmpeg 路径");
        emit logMessage("✗ FFmpeg 不可用");
        return;
    }

    if (m_videoSourcePath.isEmpty() || m_mergedOutputPath.isEmpty()) {
        setStatusMessage("请先配置原视频路径和合成视频路径");
        return;
    }

    // Ensure output dir exists
    QDir().mkpath(m_mergedOutputPath);

    setCurrentStep("合成视频+字幕");
    setIsProcessing(true);
    setProgress(0.0);
    emit logMessage("========== 步骤3：合成视频+字幕 ==========");
    m_logger->info(QString("合成: %1 → %2 (GPU=%3)")
        .arg(m_videoSourcePath, m_mergedOutputPath).arg(m_gpuAccel));

    m_mergeService->startMerge(ffmpeg, m_videoSourcePath, m_mergedOutputPath,
                                m_recursive, m_gpuAccel, m_fragmentedMp4);
}

void CustomSubtitleController::replaceOriginalVideo()
{
    if (m_isProcessing) return;

    if (m_mergedOutputPath.isEmpty() || m_videoSourcePath.isEmpty()) {
        setStatusMessage("请先配置合成视频路径和原视频路径");
        return;
    }

    setCurrentStep("替换原视频");
    setIsProcessing(true);
    setProgress(0.0);
    setProcessedCount(0);
    setTotalCount(0);
    emit logMessage("========== 步骤4：替换原视频 ==========");
    emit logMessage(QString("原视频目录: %1 (递归=%2)").arg(m_videoSourcePath).arg(m_recursive));
    emit logMessage(QString("合成视频目录: %1").arg(m_mergedOutputPath));
    m_logger->info(QString("========== 步骤4：替换原视频 =========="));
    m_logger->info(QString("原视频目录: %1 (递归=%2)").arg(m_videoSourcePath).arg(m_recursive));
    m_logger->info(QString("合成视频目录: %1").arg(m_mergedOutputPath));
    m_logger->info(QString("删除字幕=%1 | 备份原文件=%2")
        .arg(m_removeSrtAfterReplace).arg(m_backupOriginal));

    m_replaceService->startReplace(m_videoSourcePath, m_mergedOutputPath,
                                    m_recursive, m_removeSrtAfterReplace, m_backupOriginal);
}

void CustomSubtitleController::cancel()
{
    m_mergeService->cancel();
    m_replaceService->cancel();

    setIsProcessing(false);
    setStatusMessage("已取消");
    setCurrentStep("");
    setProcessedCount(0);
    setTotalCount(0);
    emit logMessage("⏹ 已取消操作");
}

void CustomSubtitleController::requestStopAfterCurrent()
{
    if (m_mergeService)
        m_mergeService->requestStopAfterCurrent();
}

void CustomSubtitleController::reset()
{
    cancel();
    setStatusMessage("");
    setCurrentStep("");
    setProgress(0.0);
    setCurrentFileProgress(0.0);
}

QString CustomSubtitleController::ffmpegPath() const
{
    return m_ffmpegPath;
}

// ── Slots ──

void CustomSubtitleController::onMatchFinished(bool success, const QString &error)
{
    if (!success) {
        setStatusMessage("步骤2失败: " + error);
        emit logMessage("✗ 步骤2失败: " + error);
        m_logger->error(QString("步骤2失败: %1").arg(error));
    } else {
        m_logger->info("步骤2完成 ✓");
    }
    setIsProcessing(false);
    setCurrentStep("");
}

void CustomSubtitleController::onMergeFinished(bool success, const QString &error)
{
    if (success) {
        setStatusMessage("步骤3完成");
        emit logMessage("✓ 步骤3：合成完成");
        m_logger->info("步骤3完成 ✓");
    } else {
        setStatusMessage(error);
        emit logMessage("✗ 步骤3失败: " + error);
        m_logger->error(QString("步骤3失败: %1").arg(error));
    }
    setIsProcessing(false);
    setCurrentStep("");
}

void CustomSubtitleController::onReplaceFinished(bool success, const QString &error)
{
    if (success) {
        setStatusMessage("步骤4完成");
        emit logMessage("✓ 步骤4：替换完成");
        m_logger->info("步骤4完成 ✓");
    } else {
        setStatusMessage(error);
        emit logMessage("✗ 步骤4失败: " + error);
        m_logger->error(QString("步骤4失败: %1").arg(error));
    }
    setIsProcessing(false);
    setCurrentStep("");
}

// ── Internal ──

void CustomSubtitleController::setStatusMessage(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged();
    }
}

void CustomSubtitleController::setCurrentStep(const QString &step)
{
    if (m_currentStep != step) {
        m_currentStep = step;
        emit currentStepChanged();
    }
}

void CustomSubtitleController::setProgress(double value)
{
    value = qBound(0.0, value, 1.0);
    if (!qFuzzyCompare(m_progress, value)) {
        m_progress = value;
        emit progressChanged();
    }
}

void CustomSubtitleController::setCurrentFileProgress(double value)
{
    value = qBound(0.0, value, 1.0);
    if (!qFuzzyCompare(m_currentFileProgress, value)) {
        m_currentFileProgress = value;
        emit currentFileProgressChanged();
    }
}

void CustomSubtitleController::setProcessedCount(int count)
{
    if (m_processedCount != count) {
        m_processedCount = count;
        emit processedCountChanged();
    }
}

void CustomSubtitleController::setTotalCount(int count)
{
    if (m_totalCount != count) {
        m_totalCount = count;
        emit totalCountChanged();
    }
}

void CustomSubtitleController::setCurrentFile(const QString &path)
{
    if (m_currentFile != path) {
        m_currentFile = path;
        emit currentFileChanged();
    }
}

void CustomSubtitleController::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        if (!processing)
            setCurrentFile("");
        emit isProcessingChanged();
    }
}
