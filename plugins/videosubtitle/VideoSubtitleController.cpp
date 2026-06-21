#include "VideoSubtitleController.h"
#include "VideoSubtitleSettings.h"
#include "WhisperService.h"
#include "TranslateService.h"
#include "FFmpegService.h"
#include "FfmpegUtils.h"
#include "SubtitleService.h"
#include "Config.h"
#include "Logger.h"
#include <QFileInfo>
#include <QDir>


VideoSubtitleController::VideoSubtitleController(PluginLogger *logger, VideoSubtitleSettings *settings, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(settings)
    , m_whisperService(new WhisperService(m_logger, this))
    , m_translateService(new TranslateService(m_logger, this))
    , m_ffmpegService(new FFmpegService(m_logger, this))
{
    connect(m_whisperService, &WhisperService::progress,
            this, &VideoSubtitleController::onWhisperProgress);
    connect(m_whisperService, &WhisperService::finished,
            this, &VideoSubtitleController::onTranscribeFinished);
    connect(m_whisperService, &WhisperService::statusUpdate,
            this, &VideoSubtitleController::logMessage);
    connect(m_translateService, &TranslateService::progress,
            this, &VideoSubtitleController::onTranslateProgress);
    connect(m_translateService, &TranslateService::finished,
            this, &VideoSubtitleController::onTranslateFinished);
    connect(m_ffmpegService, &FFmpegService::progress,
            this, &VideoSubtitleController::onFFmpegProgress);
    // NOTE: FFmpegService::finished is NOT connected here permanently.
    // It is dynamically connected to onAudioExtracted or onBurnFinished
    // depending on the current step (see processSingleFile / onTranslateFinished).

    // 确保 Settings 从 INI 加载
    m_settings->loadSettings();

    // 验证输出目录有效性
    if (m_settings->outputMode() == 1 && !m_settings->outputDir().isEmpty()) {
        if (!QDir(m_settings->outputDir()).exists()) {
            m_settings->setOutputMode(0);
            m_settings->setOutputDir({});
        }
    }
}

// Getters
QString VideoSubtitleController::inputPath() const { return m_settings->inputPath(); }
int VideoSubtitleController::inputMode() const { return m_settings->inputMode(); }
bool VideoSubtitleController::recursive() const { return m_settings->recursive(); }
QString VideoSubtitleController::sourceLanguage() const { return m_settings->sourceLanguage(); }
QString VideoSubtitleController::targetLanguage() const { return m_settings->targetLanguage(); }
int VideoSubtitleController::subtitleStyle() const { return m_settings->subtitleStyle(); }
int VideoSubtitleController::outputMode() const { return m_settings->outputMode(); }
QString VideoSubtitleController::outputDir() const { return m_settings->outputDir(); }
bool VideoSubtitleController::keepWav() const { return m_settings->keepWav(); }
bool VideoSubtitleController::keepOriginalSrt() const { return m_settings->keepOriginalSrt(); }
bool VideoSubtitleController::keepTranslatedSrt() const { return m_settings->keepTranslatedSrt(); }
QString VideoSubtitleController::statusMessage() const { return m_statusMessage; }
double VideoSubtitleController::progress() const { return m_progress; }
bool VideoSubtitleController::isProcessing() const { return m_isProcessing; }
bool VideoSubtitleController::hasRecords() const { return !m_records.isEmpty(); }
QVariantList VideoSubtitleController::records() const {
    QVariantList result;
    for (const QVariantMap &record : m_records) {
        result.append(record);
    }
    return result;
}
int VideoSubtitleController::currentStep() const { return m_currentStep; }
QString VideoSubtitleController::currentStepName() const { return m_currentStepName; }

QString VideoSubtitleController::stepNameForStep(int step)
{
    switch (step) {
    case StepExtractAudio: return "提取音频";
    case StepTranscribe:   return "语音识别";
    case StepTranslate:    return "翻译字幕";
    case StepBurnSubtitle: return "烧录字幕";
    default:               return "";
    }
}

// Setters
void VideoSubtitleController::setInputPath(const QString &path)
{
    m_settings->setInputPath(path);
    // 仅当路径是目录时才设置输出目录
    if (!path.isEmpty() && QDir(path).exists()) {
        m_settings->setOutputDir(path);
    }
    emit inputPathChanged();
}

void VideoSubtitleController::setInputMode(int mode)
{
    if (m_settings->inputMode() != mode) {
        m_settings->setInputMode(mode);
        emit inputModeChanged();
    }
}

void VideoSubtitleController::setRecursive(bool recursive)
{
    if (m_settings->recursive() != recursive) {
        m_settings->setRecursive(recursive);
        emit recursiveChanged();
    }
}

void VideoSubtitleController::setSourceLanguage(const QString &lang)
{
    if (m_settings->sourceLanguage() != lang) {
        m_settings->setSourceLanguage(lang);
        emit sourceLanguageChanged();
    }
}

void VideoSubtitleController::setTargetLanguage(const QString &lang)
{
    if (m_settings->targetLanguage() != lang) {
        m_settings->setTargetLanguage(lang);
        emit targetLanguageChanged();
    }
}

bool VideoSubtitleController::translateMusic() const
{
    return m_settings->translateMusic();
}

void VideoSubtitleController::setTranslateMusic(bool enabled)
{
    if (m_settings->translateMusic() != enabled) {
        m_settings->setTranslateMusic(enabled);
        emit translateMusicChanged();
    }
}

void VideoSubtitleController::setSubtitleStyle(int style)
{
    m_settings->setSubtitleStyle(style);
    emit subtitleStyleChanged();
}

void VideoSubtitleController::setOutputMode(int mode)
{
    if (m_settings->outputMode() != mode) {
        m_settings->setOutputMode(mode);
        emit outputModeChanged();
    }
}

void VideoSubtitleController::setOutputDir(const QString &dir)
{
    if (m_settings->outputDir() != dir) {
        m_settings->setOutputDir(dir);
        emit outputDirChanged();
    }
}

void VideoSubtitleController::setKeepWav(bool keep)
{
    m_settings->setKeepWav(keep);
    emit keepWavChanged();
}

void VideoSubtitleController::setKeepOriginalSrt(bool keep)
{
    m_settings->setKeepOriginalSrt(keep);
    emit keepOriginalSrtChanged();
}

void VideoSubtitleController::setKeepTranslatedSrt(bool keep)
{
    m_settings->setKeepTranslatedSrt(keep);
    emit keepTranslatedSrtChanged();
}

// Step control getters
bool VideoSubtitleController::enableAudioExtraction() const { return m_settings->enableAudioExtraction(); }
bool VideoSubtitleController::enableTranscribe() const { return m_settings->enableTranscribe(); }
bool VideoSubtitleController::enableTranslate() const { return m_settings->enableTranslate(); }
bool VideoSubtitleController::enableBurnSubtitle() const { return m_settings->enableBurnSubtitle(); }

// Step control setters
void VideoSubtitleController::setEnableAudioExtraction(bool enabled)
{
    if (m_settings->enableAudioExtraction() != enabled) {
        m_settings->setEnableAudioExtraction(enabled);
        emit enableAudioExtractionChanged();
    }
}
void VideoSubtitleController::setEnableTranscribe(bool enabled)
{
    if (m_settings->enableTranscribe() != enabled) {
        m_settings->setEnableTranscribe(enabled);
        emit enableTranscribeChanged();
    }
}
void VideoSubtitleController::setEnableTranslate(bool enabled)
{
    if (m_settings->enableTranslate() != enabled) {
        m_settings->setEnableTranslate(enabled);
        emit enableTranslateChanged();
    }
}
void VideoSubtitleController::setEnableBurnSubtitle(bool enabled)
{
    if (m_settings->enableBurnSubtitle() != enabled) {
        m_settings->setEnableBurnSubtitle(enabled);
        emit enableBurnSubtitleChanged();
    }
}

// Actions
void VideoSubtitleController::execute()
{
    if (m_isProcessing) return;

    if (m_settings->inputPath().isEmpty()) {
        setStatusMessage("请先选择输入路径");
        return;
    }

    // Validate tools based on which steps are enabled
    if ((m_settings->enableAudioExtraction() || m_settings->enableBurnSubtitle())
        && (m_settings->ffmpegPath().isEmpty() || !isFFmpegAvailable(m_settings->ffmpegPath(), m_logger))) {
        emit settingsRequired();
        setStatusMessage("请先在设置中配置 FFmpeg 路径");
        return;
    }

    if (m_settings->enableTranscribe()) {
        if (m_settings->whisperPath().isEmpty() || !QFileInfo::exists(m_settings->whisperPath())) {
            emit settingsRequired();
            setStatusMessage("请先在设置中配置 whisper.cpp 路径");
            return;
        }
        if (!WhisperService::isWhisperAvailable(m_settings->whisperPath(), m_logger)) {
            emit settingsRequired();
            setStatusMessage("Whisper 运行时环境异常，请检查 whisper.dll / ggml.dll 是否齐全");
            return;
        }
        if (m_settings->whisperModelPath().isEmpty() || !QFileInfo::exists(m_settings->whisperModelPath())) {
            emit settingsRequired();
            setStatusMessage("请先在设置中下载 Whisper 模型");
            return;
        }
    }

    if (m_settings->enableTranslate()) {
        int engine = m_settings->translateEngine();
        if (engine == 0 && (m_settings->apiKey().isEmpty() || m_settings->baiduAppId().isEmpty())) {
            emit settingsRequired();
            setStatusMessage("请先在设置中配置百度翻译 APP ID 和密钥");
            return;
        }
        // LibreTranslate (engine == 1) needs no API key, just a local server URL
    }

    m_logger->info(QString("===== 开始批量处理 ====="));
    m_logger->info(QString("翻译引擎: %1, 源语言: %2, 目标语言: %3")
        .arg((m_settings->translateEngine() == 0) ? "百度翻译" : (m_settings->translateEngine() == 1) ? "libretranslate" : "未知选项")
        .arg(m_settings->sourceLanguage(), m_settings->targetLanguage()));
    m_logger->info(QString("步骤: 提取音频=%1, 语音识别=%2, 翻译=%3, 烧录=%4")
        .arg(m_settings->enableAudioExtraction() ? "开" : "关")
        .arg(m_settings->enableTranscribe() ? "开" : "关")
        .arg(m_settings->enableTranslate() ? "开" : "关")
        .arg(m_settings->enableBurnSubtitle() ? "开" : "关"));

    // Collect video files
    m_pendingFiles.clear();
    m_currentFileIndex = -1;
    m_stopTargetIndex = -1;

    if (m_settings->inputMode() == 0) {
        // Single file
        if (isVideoFile(m_settings->inputPath())) {
            m_pendingFiles.append(m_settings->inputPath());
        } else {
            setStatusMessage("选择的文件不是支持的视频格式");
            return;
        }
    } else {
        // Directory mode — 按 Windows 自然排序处理，保证处理顺序和 Explorer 看到的一致
        QDir dir(m_settings->inputPath());

        if (m_settings->recursive()) {
            // 子目录按自然排序，目录优先于文件（匹配 Windows Explorer 行为）
            QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
            naturalSort(subdirs);
            for (const QFileInfo &subdirInfo : subdirs) {
                QDir subDir(subdirInfo.absoluteFilePath());
                QFileInfoList subFiles = subDir.entryInfoList(QDir::Files, QDir::NoSort);
                naturalSort(subFiles);
                for (const QFileInfo &subFile : subFiles) {
                    if (isVideoFile(subFile.fileName()))
                        m_pendingFiles.append(subFile.absoluteFilePath());
                }
            }

            // 顶层文件在子目录之后处理
            QFileInfoList topFiles = dir.entryInfoList(QDir::Files, QDir::NoSort);
            naturalSort(topFiles);
            for (const QFileInfo &file : topFiles) {
                if (isVideoFile(file.fileName()))
                    m_pendingFiles.append(file.absoluteFilePath());
            }
        } else {
            QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::NoSort);
            naturalSort(files);
            for (const QFileInfo &file : files) {
                if (isVideoFile(file.fileName()))
                    m_pendingFiles.append(file.absoluteFilePath());
            }
        }

        if (m_pendingFiles.isEmpty()) {
            setStatusMessage("所选目录中没有找到视频文件");
            return;
        }
    }

    setIsProcessing(true);
    setProgress(0.0);

    // GPU 加速设置
    {
        bool useGpu = m_settings->useGpuAccel();
        m_ffmpegService->setUseHardwareAccel(useGpu);
        m_logger->info(QString("FFmpeg GPU 加速: %1").arg(useGpu ? "开启" : "关闭"));
    }

    processNextFile();
}

void VideoSubtitleController::cancel()
{
    if (!m_isProcessing) return;

    // 先设状态为 false，防止 cancel 过程中 waitForFinished 阻塞处理事件
    // 导致异步信号回调（onTranscribeFinished/onTranslateFinished）重新进入处理流程
    setIsProcessing(false);

    m_whisperService->cancel();
    m_translateService->cancel();
    m_ffmpegService->cancel();

    setStatusMessage("已取消处理");
    setCurrentStep(StepNone, "");
    setProgress(0.0);
}

void VideoSubtitleController::clearRecords()
{
    m_records.clear();
    emit hasRecordsChanged();
    emit recordsChanged();
}

void VideoSubtitleController::reset()
{
    cancel();
    m_settings->setInputPath({});
    m_pendingFiles.clear();
    m_currentFileIndex = -1;
    m_stopTargetIndex = -1;
    m_records.clear();
    setStatusMessage("");
    setCurrentStep(StepNone, "");
    setProgress(0.0);

    emit inputPathChanged();
    emit hasRecordsChanged();
    emit recordsChanged();
}

void VideoSubtitleController::requestStopAfterCount(int count)
{
    if (count <= 0) {
        m_stopTargetIndex = -1;
        emit logMessage("  ⏹ 已取消预约停止，将处理全部文件");
        return;
    }

    m_stopTargetIndex = m_currentFileIndex + count;
    emit logMessage(QString("  ⏹ 已预约停止，再完成 %1 个文件后停止").arg(count));
}

void VideoSubtitleController::processNextFile()
{
    // 如果已取消，不再继续处理后续文件
    if (!m_isProcessing) {
        m_logger->info("处理已取消，跳过后续文件");
        return;
    }

    m_currentFileIndex++;

    // 预约停止检查
    bool stopHit = (m_stopTargetIndex >= 0 && m_currentFileIndex >= m_stopTargetIndex);
    if (stopHit && m_currentFileIndex < m_pendingFiles.size()) {
        m_logger->info(QString("预约停止: 已完成 %1 个文件，跳过剩余 %2 个")
            .arg(m_stopTargetIndex).arg(m_pendingFiles.size() - m_currentFileIndex));
        emit logMessage(QString("⏹ 预约停止: 已完成 %1 个文件，跳过剩余 %2 个")
            .arg(m_stopTargetIndex).arg(m_pendingFiles.size() - m_currentFileIndex));
        setIsProcessing(false);
        setStatusMessage(QString("预约停止，完成 %1 个文件").arg(m_stopTargetIndex));
        setCurrentStep(StepNone, "");
        return;
    }

    if (m_currentFileIndex >= m_pendingFiles.size()) {
        // All files done
        setIsProcessing(false);
        setStatusMessage(QString("处理完成，共 %1 个文件").arg(m_pendingFiles.size()));
        setCurrentStep(StepNone, "");
        return;
    }

    processSingleFile(m_pendingFiles.at(m_currentFileIndex));
}

void VideoSubtitleController::processSingleFile(const QString &videoPath)
{
    m_currentVideoPath = videoPath;
    QFileInfo fi(videoPath);
    QString baseName = fi.completeBaseName();
    QString dirPath = fi.absolutePath();

    // Determine output directory
    QString outDir;
    if (m_settings->outputMode() == 1 && !m_settings->outputDir().isEmpty()) {
        outDir = m_settings->outputDir();
    } else {
        outDir = dirPath;
    }

    // Create an intermediate folder named after the source video
    QString intermediateDir = outDir + "/" + baseName;
    QDir().mkpath(intermediateDir);

    m_currentAudioPath = intermediateDir + "/" + baseName + ".wav";
    m_currentOriginalSrtPath = intermediateDir + "/" + baseName + ".srt";
    m_currentTranslatedSrtPath = intermediateDir + "/" + baseName + "_translated.srt";
    m_currentOutputVideoPath = outDir + "/" + baseName + "_subtitled." + fi.suffix();

    setStatusMessage(QString("处理中 [%1/%2]: %3")
        .arg(m_currentFileIndex + 1)
        .arg(m_pendingFiles.size())
        .arg(fi.fileName()));

    m_logger->info(QString("开始处理视频: %1").arg(fi.fileName()));

    emit logMessage(QString("========== 开始处理: %1 ").arg(fi.fileName()));

    // [优化] 如果输出视频已存在，跳过整个文件
    if (m_settings->enableBurnSubtitle() && QFileInfo::exists(m_currentOutputVideoPath)) {
        QString reason = QString("输出文件已存在，跳过: %1").arg(m_currentOutputVideoPath);
        m_logger->info(reason);
        emit logMessage("⏭ " + fi.fileName() + " 输出已存在，跳过");
        emit logMessage(QString("========== 处理完成: %1 ").arg(fi.fileName()));
        addRecord(m_currentVideoPath, m_currentOutputVideoPath, true, "跳过（输出已存在）");
        processNextFile();
        return;
    }

    // [优化] 如果翻译字幕文件已存在，校验格式和结束时间后决定是否直接跳转到烧录
    if (m_settings->enableBurnSubtitle() && QFileInfo::exists(m_currentTranslatedSrtPath)) {
        QList<SubtitleService::SubtitleEntry> existingEntries =
            SubtitleService::parseSrt(m_currentTranslatedSrtPath);
        if (!existingEntries.isEmpty()) {
            qint64 srtLastEnd = existingEntries.last().endTime;
            qint64 videoDuration = getVideoDuration(m_settings->ffmpegPath(),
                                                                    m_currentVideoPath);
            bool timeValid = (videoDuration <= 0)
                          || (srtLastEnd <= videoDuration + 2000); // 结束时间不超出视频2秒以上
            if (timeValid) {
                m_logger->info(
                    QString("检测到已有翻译字幕 (%1 条字幕)，校验通过，直接烧录")
                        .arg(existingEntries.size()));
                emit logMessage("✓ 检测到已有翻译字幕，跳过前置步骤，直接烧录");

                setCurrentStep(StepBurnSubtitle, "烧录字幕");
                setProgress(0.0);
                disconnect(m_ffmpegService, &FFmpegService::finished, this, nullptr);
                connect(m_ffmpegService, &FFmpegService::finished,
                        this, &VideoSubtitleController::onBurnFinished);
                m_ffmpegService->startBurnSubtitles(ffmpegPath(), m_currentVideoPath,
                                                     m_currentTranslatedSrtPath,
                                                     m_currentOutputVideoPath,
                                                     defaultFontSize(),
                                                     defaultFontColor(),
                                                     defaultBorderColor(),
                                                     defaultBorderWidth());
                return;
            } else {
                m_logger->info(
                    QString("已有翻译字幕结束时间(%1 ms)超出视频时长(%2 ms)，重新处理")
                        .arg(srtLastEnd).arg(videoDuration));
                emit logMessage("⚠ 已有翻译字幕结束时间超出视频范围，重新处理");
            }
        } else {
            m_logger->info("已有翻译字幕文件为空或格式错误，重新处理: "
                               + m_currentTranslatedSrtPath);
            emit logMessage("⚠ 已有翻译字幕无效，重新处理");
        }
    }

    // [优化] 如果音频文件已存在且有效，跳过音频提取
    if (m_settings->enableAudioExtraction() && QFileInfo::exists(m_currentAudioPath)) {
        qint64 wavSize = QFileInfo(m_currentAudioPath).size();
        if (wavSize > 1024) {  // 有效音频文件至少 1KB
            m_logger->info(QString("检测到已有音频文件 (%1 bytes)，跳过提取").arg(wavSize));
            emit logMessage("✓ 检测到已有音频文件，跳过提取");
            disconnect(m_ffmpegService, &FFmpegService::finished, this, nullptr);
            connect(m_ffmpegService, &FFmpegService::finished,
                    this, &VideoSubtitleController::onAudioExtracted);
            onAudioExtracted(true, m_currentAudioPath, "");
            return;
        } else {
            m_logger->info(QString("已有音频文件过小 (%1 bytes)，重新提取").arg(wavSize));
            emit logMessage("⚠ 已有音频文件过小，重新提取");
        }
    }

    if (m_settings->enableAudioExtraction()) {
        // Step 1: Extract audio
        setCurrentStep(StepExtractAudio, "提取音频");
        setProgress(0.0);
        m_logger->info("步骤 1/4: 提取音频 → " + m_currentAudioPath);
        disconnect(m_ffmpegService, &FFmpegService::finished, this, nullptr);
        connect(m_ffmpegService, &FFmpegService::finished,
                this, &VideoSubtitleController::onAudioExtracted);
        m_ffmpegService->startExtractAudio(m_settings->ffmpegPath(), videoPath, m_currentAudioPath);
        emit logDetail("音频提取...");
    } else {
        m_logger->info("步骤 1/4: 跳过提取音频");
        emit logMessage("跳过音频提取");
        // Skip audio extraction, proceed to next enabled step
        onAudioExtracted(true, m_currentAudioPath, "");
    }
}

void VideoSubtitleController::onAudioExtracted(bool success, const QString &audioPath, const QString &error)
{
    if (!success) {
        // 日志记录完整错误详情
        m_logger->error("音频提取失败——原始错误: " + error);
        // 界面显示错误原因（error 已由 FFmpegService 翻译为中文）
        emit logMessage("✗ 音频提取失败: " + error);
        addRecord(m_currentVideoPath, "", false, "音频提取失败: " + error);
        emit logMessage(QString("========== 处理完成: %1 ").arg(QFileInfo(m_currentVideoPath).fileName()));
        processNextFile();
        return;
    }

    m_logger->info("音频提取完成: " + audioPath);
    emit logMessage("✓ 音频提取完成");

    if (m_settings->enableTranscribe()) {
        // [优化] 如果原版字幕文件已存在且有效，跳过语音识别
        if (QFileInfo::exists(m_currentOriginalSrtPath)) {
            QList<SubtitleService::SubtitleEntry> existingEntries =
                SubtitleService::parseSrt(m_currentOriginalSrtPath);
            if (!existingEntries.isEmpty()) {
                m_logger->info(QString("检测到已有原版字幕 (%1 条字幕)，跳过语音识别")
                    .arg(existingEntries.size()));
                emit logMessage("✓ 检测到已有原版字幕，跳过语音识别");
                onTranscribeFinished(true, m_currentOriginalSrtPath, "");
                return;
            } else {
                m_logger->info("已有原版字幕文件为空或格式错误，重新识别: "
                                   + m_currentOriginalSrtPath);
                emit logMessage("⚠ 已有原版字幕无效，重新识别");
            }
        }

        // Step 2: Transcribe
        setCurrentStep(StepTranscribe, "语音识别");
        setProgress(0.0);
        emit logDetail("语音识别开始...");
        m_logger->info("步骤 2/4: 语音识别中...");
        QFileInfo audioInfo(m_currentAudioPath);
        QString outputDir = audioInfo.absolutePath();
        // Pass audioSegmentDuration for virtual segment progress display
        m_whisperService->startTranscribe(m_settings->whisperPath(), m_settings->whisperModelPath(),
                                           audioPath, outputDir, m_settings->sourceLanguage(),
                                           m_settings->audioSegmentDuration());
    } else {
        m_logger->info("步骤 2/4: 跳过语音识别");
        emit logMessage("跳过语音识别");
        // Skip transcribe, proceed to next enabled step
        onTranscribeFinished(true, m_currentOriginalSrtPath, "");
    }
}

void VideoSubtitleController::onTranscribeFinished(bool success, const QString &srtPath, const QString &error)
{
    if (!success) {
        // 日志记录完整错误详情
        m_logger->error("语音识别失败——原始错误: " + error);
        // 界面显示错误原因
        emit logMessage("✗ 语音识别失败: " + error);
        addRecord(m_currentVideoPath, "", false, "语音识别失败: " + error);
        emit logMessage(QString("========== 处理完成: %1 ").arg(QFileInfo(m_currentVideoPath).fileName()));
        processNextFile();
        return;
    }

    m_currentOriginalSrtPath = srtPath;
    m_logger->info("语音识别完成，SRT: " + srtPath);
    emit logMessage("✓ 语音识别完成");

    // [优化] 去重 + 过滤环境音：移除连续重复字幕和 (xxx) 类环境音后再翻译
    {
        QList<SubtitleService::SubtitleEntry> entries =
            SubtitleService::parseSrt(m_currentOriginalSrtPath);
        if (!entries.isEmpty()) {
            int beforeCount = entries.size();

            // Step 1: 去重 — 移除连续重复的字幕文本
            QList<SubtitleService::SubtitleEntry> deduped =
                SubtitleService::deduplicate(entries);
            int dedupRemoved = beforeCount - deduped.size();
            if (dedupRemoved > 0) {
                m_logger->info(
                    QString("去重: 移除 %1 条连续重复字幕（共 %2 → %3 条）")
                        .arg(dedupRemoved).arg(beforeCount).arg(deduped.size()));
                emit logMessage(QString("✓ 去重完成: 移除 %1 条重复字幕（%2 → %3）")
                    .arg(dedupRemoved).arg(beforeCount).arg(deduped.size()));
            }

            // Step 2: 过滤环境音 — 移除 (xxx) / [xxx] / （xxx） 类环境音字幕
            QList<SubtitleService::SubtitleEntry> filtered =
                SubtitleService::filterEnvironmentSounds(deduped);
            int envRemoved = deduped.size() - filtered.size();
            if (envRemoved > 0) {
                m_logger->info(
                    QString("环境音过滤: 移除 %1 条环境音字幕（%2 → %3 条）")
                        .arg(envRemoved).arg(deduped.size()).arg(filtered.size()));
                emit logMessage(QString("✓ 环境音过滤: 移除 %1 条环境音字幕（%2 → %3）")
                    .arg(envRemoved).arg(deduped.size()).arg(filtered.size()));
            }

            // Step 3: 过滤音乐字幕（仅在关闭「翻译背景音乐」时执行）
            // 移除 ♪ ... ♪ 类音乐歌词，不翻译也不烧录
            int musicRemoved = 0;
            if (!m_settings->translateMusic()) {
                auto it = filtered.begin();
                while (it != filtered.end()) {
                    if (SubtitleService::isMusicText(it->originalText)) {
                        ++musicRemoved;
                        it = filtered.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            if (musicRemoved > 0) {
                m_logger->info(
                    QString("音乐过滤: 移除 %1 条音乐字幕（%2 → %3 条）")
                        .arg(musicRemoved).arg(filtered.size() + musicRemoved).arg(filtered.size()));
                emit logMessage(QString("✓ 音乐过滤: 移除 %1 条音乐歌词字幕（%2 → %3）")
                    .arg(musicRemoved).arg(filtered.size() + musicRemoved).arg(filtered.size()));
            }

            // Step 4: 过滤音效字幕 — 移除 *xxx* 类字幕（如 "*叮咚*"、"*门铃声*"）
            int sfxRemoved = 0;
            {
                auto it = filtered.begin();
                while (it != filtered.end()) {
                    if (SubtitleService::isSoundEffect(it->originalText)) {
                        ++sfxRemoved;
                        it = filtered.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            if (sfxRemoved > 0) {
                m_logger->info(
                    QString("音效过滤: 移除 %1 条音效字幕（%2 → %3 条）")
                        .arg(sfxRemoved).arg(filtered.size() + sfxRemoved).arg(filtered.size()));
                emit logMessage(QString("✓ 音效过滤: 移除 %1 条音效字幕（%2 → %3）")
                    .arg(sfxRemoved).arg(filtered.size() + sfxRemoved).arg(filtered.size()));
            }

            // 若有变更则写回文件
            if (dedupRemoved > 0 || envRemoved > 0 || musicRemoved > 0 || sfxRemoved > 0) {
                SubtitleService::writeSrt(m_currentOriginalSrtPath, filtered);
            } else {
                m_logger->info("字幕无需去重、环境音过滤、音乐过滤或音效过滤，保持不变");
            }
        }
    }

    // Step 3: Translate
    if (m_settings->enableTranslate()) {
        setCurrentStep(StepTranslate, "翻译字幕");
        setProgress(0.0);

        // Detect SRT language from content (not from UI setting, which only controls Whisper)
        QList<SubtitleService::SubtitleEntry> entries =
            SubtitleService::parseSrt(m_currentOriginalSrtPath);
        QString detectedLang = SubtitleService::detectLanguage(entries);
        m_logger->info(QString("SRT 语种检测结果: %1").arg(detectedLang));
        emit logMessage(QString("→ 检测到字幕语种: %1").arg(detectedLang));

        emit logDetail("翻译字幕...");
        m_logger->info("步骤 3/4: 翻译字幕中...");
        m_translateService->startTranslate(m_currentOriginalSrtPath,
                                            m_currentTranslatedSrtPath,
                                            m_settings->translateEngine(), m_settings->apiKey(),
                                            m_settings->apiUrl(), detectedLang, m_settings->targetLanguage(),
                                            m_settings->baiduAppId());
    } else {
        m_logger->info("步骤 3/4: 跳过翻译");
        emit logMessage("跳过翻译");
        // No translation, treat original as translated and skip to burning
        m_currentTranslatedSrtPath = m_currentOriginalSrtPath;
        onTranslateFinished(true, m_currentOriginalSrtPath, "");
    }
}

void VideoSubtitleController::onTranslateFinished(bool success, const QString &srtPath, const QString &error)
{
    if (!success) {
        // 日志记录完整错误详情
        m_logger->error("翻译失败——原始错误: " + error);
        // 界面显示错误原因
        emit logMessage("✗ 翻译失败: " + error);
        addRecord(m_currentVideoPath, "", false, "翻译失败: " + error);
        emit logMessage(QString("========== 处理完成: %1 ").arg(QFileInfo(m_currentVideoPath).fileName()));
        processNextFile();
        return;
    }

    m_currentTranslatedSrtPath = srtPath;
    m_logger->info("翻译完成，SRT: " + srtPath);
    emit logMessage("✓ 翻译完成");

    // Step 4: Burn subtitles
    if (m_settings->enableBurnSubtitle()) {
        setCurrentStep(StepBurnSubtitle, "烧录字幕");
        setProgress(0.0);
        emit logDetail("烧录字幕...");
        m_logger->info("步骤 4/4: 烧录字幕中...");
        disconnect(m_ffmpegService, &FFmpegService::finished, this, nullptr);
        connect(m_ffmpegService, &FFmpegService::finished,
                this, &VideoSubtitleController::onBurnFinished);
        m_ffmpegService->startBurnSubtitles(m_settings->ffmpegPath(), m_currentVideoPath,
                                             m_currentTranslatedSrtPath,
                                             m_currentOutputVideoPath,
                                             m_settings->defaultFontSize(),
                                             m_settings->defaultFontColor(),
                                             m_settings->defaultBorderColor(),
                                             m_settings->defaultBorderWidth());
    } else {
        m_logger->info("步骤 4/4: 跳过烧录字幕");
        emit logMessage("跳过烧录字幕");
        // No burn, finalize with SRT output
        onBurnFinished(true, m_currentTranslatedSrtPath, "");
    }
}

void VideoSubtitleController::onBurnFinished(bool success, const QString &outputPath, const QString &error)
{
    if (success) {
        m_logger->info(QString("烧录完成: %1 → %2").arg(m_currentVideoPath, outputPath));
        emit logMessage("✓ 烧录完成");
        finalizeCurrentFile();
    } else {
        // 日志记录完整错误详情
        m_logger->error("烧录字幕失败——原始错误: " + error);
        // 界面显示错误原因
        emit logMessage("✗ 烧录字幕失败: " + error);
        addRecord(m_currentVideoPath, "", false, "烧录字幕失败: " + error);
        emit logMessage(QString("========== 处理完成: %1 ").arg(QFileInfo(m_currentVideoPath).fileName()));
        processNextFile();
    }
}

void VideoSubtitleController::finalizeCurrentFile()
{
    QFileInfo fi(m_currentVideoPath);
    QString baseName = fi.completeBaseName();
    QString outDir = (m_settings->outputMode() == 1 && !m_settings->outputDir().isEmpty())
                         ? m_settings->outputDir()
                         : fi.absolutePath();
    QString intermediateDir = outDir + "/" + baseName;

    // Track the primary output for the record
    QString outputPath;
    bool hasAnyOutput = false;

    // WAV — keep or discard
    if (m_settings->enableAudioExtraction() && QFileInfo::exists(m_currentAudioPath)) {
        if (m_settings->keepWav()) {
            outputPath = m_currentAudioPath;
            m_logger->info("输出音频: " + m_currentAudioPath);
            emit logMessage("✓ 生成音频: " + m_currentAudioPath);
            hasAnyOutput = true;
        } else {
            QFile::remove(m_currentAudioPath);
            m_logger->info("丢弃音频: " + m_currentAudioPath);
        }
    }

    // Original SRT — keep or discard
    if (m_settings->enableTranscribe() && QFileInfo::exists(m_currentOriginalSrtPath)) {
        if (m_settings->keepOriginalSrt()) {
            outputPath = m_currentOriginalSrtPath;
            m_logger->info("输出字幕: " + m_currentOriginalSrtPath);
            emit logMessage("✓ 生成字幕: " + m_currentOriginalSrtPath);
            hasAnyOutput = true;
        } else {
            QFile::remove(m_currentOriginalSrtPath);
            m_logger->info("丢弃字幕: " + m_currentOriginalSrtPath);
        }
    }

    // Translated SRT — keep or discard
    if (m_settings->enableTranslate() && QFileInfo::exists(m_currentTranslatedSrtPath)) {
        if (m_settings->keepTranslatedSrt()) {
            outputPath = m_currentTranslatedSrtPath;
            m_logger->info("输出翻译字幕: " + m_currentTranslatedSrtPath);
            emit logMessage("✓ 生成翻译字幕: " + m_currentTranslatedSrtPath);
            hasAnyOutput = true;
        } else {
            QFile::remove(m_currentTranslatedSrtPath);
            m_logger->info("丢弃翻译字幕: " + m_currentTranslatedSrtPath);
        }
    }

    // Burn output video is always kept (main output)
    if (m_settings->enableBurnSubtitle() && QFileInfo::exists(m_currentOutputVideoPath)) {
        outputPath = m_currentOutputVideoPath;
        m_logger->info("输出视频: " + outputPath);
        emit logMessage("✓ 生成视频: " + outputPath);
        hasAnyOutput = true;
    }

    // Record result
    if (hasAnyOutput && QFileInfo::exists(outputPath)) {
        addRecord(m_currentVideoPath, outputPath, true, "已完成");
        emit logMessage(QString("========== 处理完成: %1 ").arg(outputPath));
    } else if (hasAnyOutput) {
        addRecord(m_currentVideoPath, outputPath, true, "已完成（中间文件）");
        emit logMessage(QString("========== 处理完成: %1 ").arg(outputPath));
    } else {
        addRecord(m_currentVideoPath, "", false, "没有文件产出");
        emit logMessage("✗ 没有文件产出");
    }

    // Remove intermediate folder if no files are kept inside it
    QDir intDir(intermediateDir);
    if (intDir.exists()) {
        QStringList remaining = intDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        if (remaining.isEmpty()) {
            intDir.removeRecursively();
        }
    }

    processNextFile();
}

void VideoSubtitleController::onWhisperProgress(double value)
{
    int stepsBefore = m_settings->enableAudioExtraction() ? 1 : 0;
    int totalSteps = (m_settings->enableAudioExtraction() ? 1 : 0)
                   + (m_settings->enableTranscribe() ? 1 : 0)
                   + (m_settings->enableTranslate() ? 1 : 0)
                   + (m_settings->enableBurnSubtitle() ? 1 : 0);
    if (totalSteps == 0) totalSteps = 1;
    double stepWeight = 1.0 / totalSteps;
    setProgress(stepsBefore * stepWeight + value * stepWeight);
}

void VideoSubtitleController::onTranslateProgress(double value)
{
    int stepsBefore = (m_settings->enableAudioExtraction() ? 1 : 0)
                    + (m_settings->enableTranscribe() ? 1 : 0);
    int totalSteps = (m_settings->enableAudioExtraction() ? 1 : 0)
                   + (m_settings->enableTranscribe() ? 1 : 0)
                   + (m_settings->enableTranslate() ? 1 : 0)
                   + (m_settings->enableBurnSubtitle() ? 1 : 0);
    if (totalSteps == 0) totalSteps = 1;
    double stepWeight = 1.0 / totalSteps;
    setProgress(stepsBefore * stepWeight + value * stepWeight);
}

void VideoSubtitleController::onFFmpegProgress(double value)
{
    double stepWeight = 1.0;
    int totalSteps = (m_settings->enableAudioExtraction() ? 1 : 0)
                   + (m_settings->enableTranscribe() ? 1 : 0)
                   + (m_settings->enableTranslate() ? 1 : 0)
                   + (m_settings->enableBurnSubtitle() ? 1 : 0);
    if (totalSteps == 0) totalSteps = 1;
    stepWeight = 1.0 / totalSteps;

    if (m_currentStep == StepExtractAudio) {
        setProgress(0.0 + value * stepWeight);
    } else if (m_currentStep == StepBurnSubtitle) {
        int stepsBefore = (m_settings->enableAudioExtraction() ? 1 : 0)
                        + (m_settings->enableTranscribe() ? 1 : 0)
                        + (m_settings->enableTranslate() ? 1 : 0);
        setProgress(stepsBefore * stepWeight + value * stepWeight);
    }
}

// Private helpers
void VideoSubtitleController::setStatusMessage(const QString &message)
{
    if (m_statusMessage != message) {
        m_statusMessage = message;
        emit statusMessageChanged();
    }
}

void VideoSubtitleController::setCurrentStep(int step, const QString &stepName)
{
    if (m_currentStep != step) {
        m_currentStep = step;
        m_currentStepName = stepName;
        emit currentStepChanged();
    }
}

void VideoSubtitleController::setProgress(double value)
{
    value = qBound(0.0, value, 1.0);
    if (!qFuzzyCompare(m_progress, value)) {
        m_progress = value;
        emit progressChanged();
    }
}

void VideoSubtitleController::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        emit isProcessingChanged();
    }
}

void VideoSubtitleController::addRecord(const QString &originalPath, const QString &outputPath,
                                         bool success, const QString &status)
{
    QFileInfo fi(originalPath);
    QVariantMap record;
    record["originalPath"] = originalPath;
    record["originalName"] = fi.fileName();
    record["outputPath"] = outputPath;
    record["outputName"] = success ? QFileInfo(outputPath).fileName() : "";
    record["success"] = success;
    record["status"] = status;

    m_records.append(record);
    emit hasRecordsChanged();
    emit recordsChanged();
}

// ============================================================
//  All private helpers below now delegate to VideoSubtitleSettings
// ============================================================
QString VideoSubtitleController::ffmpegPath() const
{
    return m_settings->ffmpegPath();
}

QString VideoSubtitleController::whisperPath() const
{
    return m_settings->whisperPath();
}

QString VideoSubtitleController::whisperModelPath() const
{
    return m_settings->whisperModelPath();
}

QString VideoSubtitleController::apiKey() const
{
    return m_settings->apiKey();
}

QString VideoSubtitleController::apiUrl() const
{
    return m_settings->apiUrl();
}

QString VideoSubtitleController::baiduAppId() const
{
    return m_settings->baiduAppId();
}

int VideoSubtitleController::audioSegmentDuration() const
{
    return m_settings->audioSegmentDuration();
}

int VideoSubtitleController::translateEngine() const
{
    return m_settings->translateEngine();
}

int VideoSubtitleController::defaultFontSize() const
{
    return m_settings->defaultFontSize();
}

QString VideoSubtitleController::defaultFontColor() const
{
    return m_settings->defaultFontColor();
}

QString VideoSubtitleController::defaultBorderColor() const
{
    return m_settings->defaultBorderColor();
}

int VideoSubtitleController::defaultBorderWidth() const
{
    return m_settings->defaultBorderWidth();
}
