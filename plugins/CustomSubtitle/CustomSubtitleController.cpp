#include "CustomSubtitleController.h"
#include "CustomSubtitlePlugin.h"
#include "CustomSubtitleSettings.h"
#include "SubtitleMatcher.h"
#include "FFmpegMergeService.h"
#include "FfmpegUtils.h"
#include "VideoReplaceService.h"
#include "SubBrowserController.h"
#include "Config.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QProcess>

CustomSubtitleController::CustomSubtitleController(PluginLogger *logger, CustomSubtitleSettings *settings, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(settings)
    , m_matcher(new SubtitleMatcher(m_logger, this))
    , m_mergeService(new FFmpegMergeService(m_logger, this))
    , m_replaceService(new VideoReplaceService(m_logger, this))
    , m_browserController(new SubBrowserController(m_logger, this))
{
    // Connect browser controller signals
    connect(m_browserController, &SubBrowserController::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_browserController, &SubBrowserController::downloadPathChanged,
            this, [this]() {
        // 同步下载路径到 SubBrowserController
        m_browserController->setDownloadPath(m_settings->subtitleDownloadPath());
    });
    connect(m_browserController, &SubBrowserController::searchingChanged,
            this, [this]() {
        if (m_browserController->searching())
            setCurrentStep(StepSearch);
        else if (m_currentStep == StepSearch)
            setCurrentStep(StepNone);
    });

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
    connect(m_matcher, &SubtitleMatcher::scanFinished,
            this, [this](int matched) {
        setTotalCount(matched);
    });

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

    // Load persisted paths from Settings
    m_settings->loadSettings();
    // 同步初始下载路径到 SubBrowserController
    if (m_browserController && !m_settings->subtitleDownloadPath().isEmpty())
        m_browserController->setDownloadPath(m_settings->subtitleDownloadPath());
}

CustomSubtitleController::~CustomSubtitleController() = default;

// ── Getters ──
QString CustomSubtitleController::subtitleDownloadPath() const { return m_settings->subtitleDownloadPath(); }
QString CustomSubtitleController::videoSourcePath() const { return m_settings->videoSourcePath(); }
bool CustomSubtitleController::recursive() const { return m_settings->recursive(); }
QString CustomSubtitleController::mergedOutputPath() const { return m_settings->mergedOutputPath(); }
bool CustomSubtitleController::gpuAccel() const { return m_settings->gpuAccel(); }
bool CustomSubtitleController::removeSrtAfterReplace() const { return m_removeSrtAfterReplace; }
bool CustomSubtitleController::weakMatch() const { return m_settings->weakMatch(); }
QString CustomSubtitleController::statusMessage() const { return m_statusMessage; }
double CustomSubtitleController::progress() const { return m_progress; }
double CustomSubtitleController::currentFileProgress() const { return m_currentFileProgress; }
bool CustomSubtitleController::isProcessing() const { return m_isProcessing; }
int CustomSubtitleController::currentStep() const { return m_currentStep; }
int CustomSubtitleController::processedCount() const { return m_processedCount; }
int CustomSubtitleController::totalCount() const { return m_totalCount; }
QString CustomSubtitleController::currentFile() const { return m_currentFile; }
SubBrowserController* CustomSubtitleController::browserController() const { return m_browserController; }

// ── Setters ──
void CustomSubtitleController::setSubtitleDownloadPath(const QString &path)
{
    if (m_settings->subtitleDownloadPath() != path) {
        m_settings->setSubtitleDownloadPath(path);
        emit subtitleDownloadPathChanged();
        // 同步到 SubBrowserController
        if (m_browserController)
            m_browserController->setDownloadPath(path);
    }
}

void CustomSubtitleController::setVideoSourcePath(const QString &path)
{
    if (m_settings->videoSourcePath() != path) {
        m_settings->setVideoSourcePath(path);
        emit videoSourcePathChanged();
    }
}

void CustomSubtitleController::setRecursive(bool recursive)
{
    if (m_settings->recursive() != recursive) {
        m_settings->setRecursive(recursive);
        emit recursiveChanged();
    }
}

void CustomSubtitleController::setMergedOutputPath(const QString &path)
{
    if (m_settings->mergedOutputPath() != path) {
        m_settings->setMergedOutputPath(path);
        emit mergedOutputPathChanged();
    }
}

void CustomSubtitleController::setFfmpegPath(const QString &path)
{
    if (m_settings->ffmpegPath() != path) {
        m_settings->setFfmpegPath(path);
        emit ffmpegPathChanged();
    }
}

void CustomSubtitleController::setGpuAccel(bool enable)
{
    if (m_settings->gpuAccel() != enable) {
        m_settings->setGpuAccel(enable);
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

void CustomSubtitleController::setWeakMatch(bool weak)
{
    if (m_settings->weakMatch() != weak) {
        m_settings->setWeakMatch(weak);
        emit weakMatchChanged();
    }
}

// ── Preprocessing ──

QStringList CustomSubtitleController::enabledPreprocessors() const
{
    return m_settings->enabledPreprocessors();
}

void CustomSubtitleController::setEnabledPreprocessors(const QStringList &ops)
{
    if (m_settings->enabledPreprocessors() != ops) {
        m_settings->setEnabledPreprocessors(ops);
        emit enabledPreprocessorsChanged();
    }
}

// ── Actions ──

void CustomSubtitleController::matchAndMoveSubtitles()
{
    if (m_isProcessing) return;

    if (m_settings->subtitleDownloadPath().isEmpty() || m_settings->videoSourcePath().isEmpty()) {
        setStatusMessage("请先配置字幕下载路径和原视频路径");
        return;
    }

    setCurrentStep(StepMatch);
    setProgress(0.0);
    setProcessedCount(0);
    setTotalCount(0);
    emit logMessage("========== 步骤2：匹配并移动字幕 ==========");

    m_logger->info(QString("匹配字幕: %1 → %2 (递归=%3)")
        .arg(m_settings->subtitleDownloadPath(), m_settings->videoSourcePath()).arg(m_settings->recursive()));

    m_matcher->startMatchAsync(m_settings->subtitleDownloadPath(), m_settings->videoSourcePath(),
                                m_settings->recursive(), {}, m_settings->enabledPreprocessors());
}

void CustomSubtitleController::mergeSubtitleToVideo()
{
    if (m_isProcessing) return;

    QString ffmpeg = m_settings->ffmpegPath();
    if (ffmpeg.isEmpty() || !isFFmpegAvailable(ffmpeg, m_logger)) {
        setStatusMessage("FFmpeg 不可用，请在自定义字幕设置中配置 FFmpeg 路径");
        emit logMessage("✗ FFmpeg 不可用");
        return;
    }

    if (m_settings->videoSourcePath().isEmpty() || m_settings->mergedOutputPath().isEmpty()) {
        setStatusMessage("请先配置原视频路径和合成视频路径");
        return;
    }

    // Ensure output dir exists
    QDir().mkpath(m_settings->mergedOutputPath());

    m_shutdownAfterStop = false;
    setCurrentStep(StepMerge);
    setProgress(0.0);
    emit logMessage("========== 步骤3：合成视频+字幕 ==========");
    m_logger->info(QString("合成: %1 → %2 (GPU=%3)")
        .arg(m_settings->videoSourcePath(), m_settings->mergedOutputPath()).arg(m_settings->gpuAccel()));

    m_mergeService->startMerge(ffmpeg, m_settings->videoSourcePath(), m_settings->mergedOutputPath(),
                                m_settings->recursive(), m_settings->gpuAccel());
}

void CustomSubtitleController::replaceOriginalVideo()
{
    if (m_isProcessing) return;

    if (m_settings->mergedOutputPath().isEmpty() || m_settings->videoSourcePath().isEmpty()) {
        setStatusMessage("请先配置合成视频路径和原视频路径");
        return;
    }

    setCurrentStep(StepReplace);
    setProgress(0.0);
    setProcessedCount(0);
    setTotalCount(0);
    emit logMessage("========== 步骤4：替换原视频 ==========");
    emit logMessage(QString("原视频目录: %1 (递归=%2)").arg(m_settings->videoSourcePath()).arg(m_settings->recursive()));
    emit logMessage(QString("合成视频目录: %1").arg(m_settings->mergedOutputPath()));
    m_logger->info(QString("========== 步骤4：替换原视频 =========="));
    m_logger->info(QString("原视频目录: %1 (递归=%2)").arg(m_settings->videoSourcePath()).arg(m_settings->recursive()));
    m_logger->info(QString("合成视频目录: %1").arg(m_settings->mergedOutputPath()));
    m_logger->info(QString("删除字幕=%1 | 名称弱匹配=%2")
        .arg(m_removeSrtAfterReplace).arg(m_settings->weakMatch()));

    m_replaceService->startReplace(m_settings->videoSourcePath(), m_settings->mergedOutputPath(),
                                    m_settings->recursive(), m_removeSrtAfterReplace, m_settings->weakMatch());
}

void CustomSubtitleController::cancel()
{
    if (m_currentStep == StepMatch) {
        m_matcher->requestStop();
        m_mergeService->cancel();
        m_replaceService->cancel();
        m_gracefulStopRequested = true;
        emit stopRequestedChanged();
        setStatusMessage("正在停止…");
        emit logMessage("⏹ 步骤2：请求停止，完成当前字幕任务后停止");
    } else if (m_currentStep == StepReplace) {
        m_matcher->cancel();
        m_mergeService->cancel();
        m_replaceService->requestStop();
        m_gracefulStopRequested = true;
        emit stopRequestedChanged();
        setStatusMessage("正在停止…");
        emit logMessage("⏹ 步骤4：请求停止，完成当前替换任务后停止");
    } else {
        // Step 3 or unknown → aggressive
        m_gracefulStopRequested = false;
        emit stopRequestedChanged();
        m_matcher->cancel();
        m_mergeService->cancel();
        m_replaceService->cancel();

        setStatusMessage("已取消");
        setCurrentStep(StepNone);
        setProcessedCount(0);
        setTotalCount(0);
        emit logMessage("⏹ 已取消操作");
    }
}

void CustomSubtitleController::requestStopAfterCount(int count, bool shutdown)
{
    m_shutdownAfterStop = shutdown;
    if (m_mergeService)
        m_mergeService->requestStopAfterCount(count);

    if (count <= 0) {
        QString action = shutdown ? "关机" : "停止";
        emit logMessage(QString("  ⏹ 完成全部后%1").arg(action));
    } else {
        QString action = shutdown ? "关机" : "停止";
        emit logMessage(QString("  ⏹ 再完成 %1 个后%2").arg(count).arg(action));
    }
}

void CustomSubtitleController::reset()
{
    cancel();
    m_gracefulStopRequested = false;
    m_shutdownAfterStop = false;
    emit stopRequestedChanged();
    setStatusMessage("");
    setCurrentStep(StepNone);
    setProgress(0.0);
    setCurrentFileProgress(0.0);
}

QString CustomSubtitleController::ffmpegPath() const
{
    return m_settings->ffmpegPath();
}

// ── Slots ──

void CustomSubtitleController::onMatchFinished(bool success, const QString &error)
{
    if (m_gracefulStopRequested) {
        m_gracefulStopRequested = false;
        emit stopRequestedChanged();
        setStatusMessage("已停止");
        emit logMessage("⏹ 步骤2：已停止（已完成当前字幕任务）");
        m_logger->info("步骤2：已停止");
    } else if (!success) {
        setStatusMessage("步骤2失败: " + error);
        emit logMessage("✗ 步骤2失败: " + error);
        m_logger->error(QString("步骤2失败: %1").arg(error));
    } else {
        setStatusMessage("步骤2完成");
        emit logMessage("✓ 步骤2：匹配并移动字幕完成");
        m_logger->info("步骤2完成 ✓");
    }
    setCurrentStep(StepNone);
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
    setCurrentStep(StepNone);

    if (m_shutdownAfterStop) {
        m_shutdownAfterStop = false;
        emit logMessage("  ⏹ 预约任务已完成，系统将在 5 秒后关机");
        QProcess::startDetached("shutdown", {"/s", "/t", "5"});
    }
}

void CustomSubtitleController::onReplaceFinished(bool success, const QString &error)
{
    if (m_gracefulStopRequested) {
        m_gracefulStopRequested = false;
        emit stopRequestedChanged();
        setStatusMessage("已停止");
        emit logMessage("⏹ 步骤4：已停止（已完成当前任务的备份和替换）");
        m_logger->info("步骤4：已停止");
    } else if (success) {
        setStatusMessage("步骤4完成");
        emit logMessage("✓ 步骤4：替换完成");
        m_logger->info("步骤4完成 ✓");
    } else {
        setStatusMessage(error);
        emit logMessage("✗ 步骤4失败: " + error);
        m_logger->error(QString("步骤4失败: %1").arg(error));
    }
    setCurrentStep(StepNone);
}

// ── Internal ──

void CustomSubtitleController::setStatusMessage(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged();
    }
}

void CustomSubtitleController::setCurrentStep(int step)
{
    if (m_currentStep != step) {
        m_currentStep = step;
        emit currentStepChanged();
    }
    // isProcessing derives from currentStep: StepNone → false, anything else → true
    bool processing = (step != StepNone);
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        emit isProcessingChanged();
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
