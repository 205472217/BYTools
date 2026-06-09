#include "CustomSubtitleController.h"
#include "SubtitleMatcher.h"
#include "FFmpegMergeService.h"
#include "VideoReplaceService.h"
#include "PluginLogger.h"
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

// Shared config.ini path (same as videosubtitle plugin)
static QString pluginConfigFilePath()
{
    return QCoreApplication::applicationDirPath() + "/config.ini";
}

// Helper: return QSettings for the shared [VideoSubtitle] section
static QSettings& sharedSettings()
{
    static QSettings s(pluginConfigFilePath(), QSettings::IniFormat);
    static bool groupSet = false;
    if (!groupSet) {
        s.beginGroup("VideoSubtitle");
        groupSet = true;
    }
    return s;
}

CustomSubtitleController::CustomSubtitleController(QObject *parent)
    : QObject(parent)
    , m_matcher(new SubtitleMatcher(this))
    , m_mergeService(new FFmpegMergeService(this))
    , m_replaceService(new VideoReplaceService(this))
{
    // Connect matcher signals
    connect(m_matcher, &SubtitleMatcher::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_matcher, &SubtitleMatcher::progress,
            this, &CustomSubtitleController::setProgress);
    connect(m_matcher, &SubtitleMatcher::finished,
            this, &CustomSubtitleController::onMatchFinished);

    // Connect merge service signals
    connect(m_mergeService, &FFmpegMergeService::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_mergeService, &FFmpegMergeService::progress,
            this, &CustomSubtitleController::setProgress);
    connect(m_mergeService, &FFmpegMergeService::finished,
            this, &CustomSubtitleController::onMergeFinished);

    // Connect replace service signals
    connect(m_replaceService, &VideoReplaceService::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_replaceService, &VideoReplaceService::progress,
            this, &CustomSubtitleController::setProgress);
    connect(m_replaceService, &VideoReplaceService::finished,
            this, &CustomSubtitleController::onReplaceFinished);

    // Load persisted paths
    QSettings &s = sharedSettings();
    s.sync();
    m_subtitleDownloadPath = s.value("customSubtitleDownloadPath").toString();
    m_videoSourcePath = s.value("customVideoSourcePath").toString();
    m_mergedOutputPath = s.value("customMergedOutputPath").toString();
    m_recursive = s.value("customRecursive", false).toBool();
    m_gpuAccel = s.value("customGpuAccel", false).toBool();
}

CustomSubtitleController::~CustomSubtitleController() = default;

// ── Getters ──
QString CustomSubtitleController::subtitleDownloadPath() const { return m_subtitleDownloadPath; }
QString CustomSubtitleController::videoSourcePath() const { return m_videoSourcePath; }
bool CustomSubtitleController::recursive() const { return m_recursive; }
QString CustomSubtitleController::mergedOutputPath() const { return m_mergedOutputPath; }
bool CustomSubtitleController::gpuAccel() const { return m_gpuAccel; }
bool CustomSubtitleController::removeSrtAfterReplace() const { return m_removeSrtAfterReplace; }
bool CustomSubtitleController::backupOriginal() const { return m_backupOriginal; }
QString CustomSubtitleController::statusMessage() const { return m_statusMessage; }
double CustomSubtitleController::progress() const { return m_progress; }
bool CustomSubtitleController::isProcessing() const { return m_isProcessing; }
QString CustomSubtitleController::currentStep() const { return m_currentStep; }

// ── Setters ──
void CustomSubtitleController::setSubtitleDownloadPath(const QString &path)
{
    if (m_subtitleDownloadPath != path) {
        m_subtitleDownloadPath = path;
        sharedSettings().setValue("customSubtitleDownloadPath", path);
        sharedSettings().sync();
        emit subtitleDownloadPathChanged();
    }
}

void CustomSubtitleController::setVideoSourcePath(const QString &path)
{
    if (m_videoSourcePath != path) {
        m_videoSourcePath = path;
        sharedSettings().setValue("customVideoSourcePath", path);
        sharedSettings().sync();
        emit videoSourcePathChanged();
    }
}

void CustomSubtitleController::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        sharedSettings().setValue("customRecursive", recursive);
        sharedSettings().sync();
        emit recursiveChanged();
    }
}

void CustomSubtitleController::setMergedOutputPath(const QString &path)
{
    if (m_mergedOutputPath != path) {
        m_mergedOutputPath = path;
        sharedSettings().setValue("customMergedOutputPath", path);
        sharedSettings().sync();
        emit mergedOutputPathChanged();
    }
}

void CustomSubtitleController::setGpuAccel(bool enable)
{
    if (m_gpuAccel != enable) {
        m_gpuAccel = enable;
        sharedSettings().setValue("customGpuAccel", enable);
        sharedSettings().sync();
        emit gpuAccelChanged();
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

    PluginLogger::info(QString("匹配字幕: %1 → %2 (递归=%3)")
        .arg(m_subtitleDownloadPath, m_videoSourcePath).arg(m_recursive));

    // Run matching (blocking in current thread for simplicity; for large dirs use QThread)
    QList<SubtitleMatcher::MatchResult> results = m_matcher->matchSubtitles(
        m_subtitleDownloadPath, m_videoSourcePath, m_recursive, {});

    if (results.isEmpty()) {
        setStatusMessage("匹配结束：无匹配结果");
        setIsProcessing(false);
        setCurrentStep("");
        onMatchFinished(false, "无匹配结果");
        return;
    }

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
    if (ffmpeg.isEmpty() || !FFmpegMergeService::isFFmpegAvailable(ffmpeg)) {
        setStatusMessage("FFmpeg 不可用，请先在视频字幕翻译设置中配置 FFmpeg 路径");
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
    PluginLogger::info(QString("合成: %1 → %2 (GPU=%3)")
        .arg(m_videoSourcePath, m_mergedOutputPath).arg(m_gpuAccel));

    m_mergeService->startMerge(ffmpeg, m_videoSourcePath, m_mergedOutputPath,
                                m_recursive, m_gpuAccel);
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
    emit logMessage("========== 步骤4：替换原视频 ==========");
    PluginLogger::info(QString("替换: %1 ← %2 (删除字幕=%3, 备份=%4)")
        .arg(m_videoSourcePath, m_mergedOutputPath)
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
    emit logMessage("⏹ 已取消操作");
}

void CustomSubtitleController::reset()
{
    cancel();
    setStatusMessage("");
    setCurrentStep("");
    setProgress(0.0);
}

QString CustomSubtitleController::ffmpegPath() const
{
    QSettings &s = sharedSettings();
    s.sync();
    return s.value("ffmpegPath").toString();
}

// ── Slots ──

void CustomSubtitleController::onMatchFinished(bool success, const QString &error)
{
    if (!success) {
        setStatusMessage("步骤2失败: " + error);
        emit logMessage("✗ 步骤2失败: " + error);
    }
    setIsProcessing(false);
    setCurrentStep("");
}

void CustomSubtitleController::onMergeFinished(bool success, const QString &error)
{
    Q_UNUSED(error)
    if (success) {
        setStatusMessage("步骤3完成");
        emit logMessage("✓ 步骤3：合成完成");
    } else {
        setStatusMessage("步骤3失败");
        emit logMessage("✗ 步骤3失败");
    }
    setIsProcessing(false);
    setCurrentStep("");
}

void CustomSubtitleController::onReplaceFinished(bool success, const QString &error)
{
    Q_UNUSED(error)
    if (success) {
        setStatusMessage("步骤4完成");
        emit logMessage("✓ 步骤4：替换完成");
    } else {
        setStatusMessage("步骤4失败");
        emit logMessage("✗ 步骤4失败");
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

void CustomSubtitleController::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        emit isProcessingChanged();
    }
}
