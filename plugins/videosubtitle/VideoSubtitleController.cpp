#include "VideoSubtitleController.h"
#include "WhisperService.h"
#include "TranslateService.h"
#include "FFmpegService.h"
#include "SubtitleService.h"
#include "PluginLogger.h"
#include <QCoreApplication>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

// Helper: 返回指向 config.ini [VideoSubtitle] 节的 QSettings（静态全局避免拷贝）
static QSettings& pluginSettings()
{
    static QSettings s(pluginConfigFilePath(), QSettings::IniFormat);
    static bool groupSet = false;
    if (!groupSet) {
        s.beginGroup("VideoSubtitle");
        groupSet = true;
    }
    return s;
}

VideoSubtitleController::VideoSubtitleController(QObject *parent)
    : QObject(parent)
    , m_whisperService(new WhisperService(this))
    , m_translateService(new TranslateService(this))
    , m_ffmpegService(new FFmpegService(this))
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

    // 加载持久化的输出目录设置
    QSettings &s = pluginSettings();
    m_outputDir = s.value("outputDir").toString();
    m_outputMode = s.value("outputMode", 0).toInt();
    if (m_outputMode == 1 && !m_outputDir.isEmpty()) {
        // 如果保存的目录不存在，回退到同目录模式
        if (!QDir(m_outputDir).exists()) {
            m_outputMode = 0;
            m_outputDir.clear();
        }
    } else {
        m_outputDir.clear();
    }
}

// Getters
QString VideoSubtitleController::inputPath() const { return m_inputPath; }
int VideoSubtitleController::inputMode() const { return m_inputMode; }
bool VideoSubtitleController::recursive() const { return m_recursive; }
QString VideoSubtitleController::sourceLanguage() const { return m_sourceLanguage; }
QString VideoSubtitleController::targetLanguage() const { return m_targetLanguage; }
int VideoSubtitleController::subtitleStyle() const { return pluginSettings().value("subtitleStyle", 0).toInt(); }
int VideoSubtitleController::outputMode() const { return m_outputMode; }
QString VideoSubtitleController::outputDir() const { return m_outputDir; }
bool VideoSubtitleController::keepWav() const { return pluginSettings().value("keepWav", true).toBool(); }
bool VideoSubtitleController::keepOriginalSrt() const { return pluginSettings().value("keepOriginalSrt", true).toBool(); }
bool VideoSubtitleController::keepTranslatedSrt() const { return pluginSettings().value("keepTranslatedSrt", true).toBool(); }
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
QString VideoSubtitleController::currentStep() const { return m_currentStep; }

// Setters
void VideoSubtitleController::setInputPath(const QString &path)
{
    if (m_inputPath != path) {
        m_inputPath = path;
        emit inputPathChanged();
    }
}

void VideoSubtitleController::setInputMode(int mode)
{
    if (m_inputMode != mode) {
        m_inputMode = mode;
        emit inputModeChanged();
    }
}

void VideoSubtitleController::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        emit recursiveChanged();
    }
}

void VideoSubtitleController::setSourceLanguage(const QString &lang)
{
    if (m_sourceLanguage != lang) {
        m_sourceLanguage = lang;
        emit sourceLanguageChanged();
    }
}

void VideoSubtitleController::setTargetLanguage(const QString &lang)
{
    if (m_targetLanguage != lang) {
        m_targetLanguage = lang;
        emit targetLanguageChanged();
    }
}

void VideoSubtitleController::setSubtitleStyle(int style)
{
    pluginSettings().setValue("subtitleStyle", style);
    emit subtitleStyleChanged();
}

void VideoSubtitleController::setOutputMode(int mode)
{
    if (m_outputMode != mode) {
        m_outputMode = mode;
        pluginSettings().setValue("outputMode", mode);
        emit outputModeChanged();
    }
}

void VideoSubtitleController::setOutputDir(const QString &dir)
{
    if (m_outputDir != dir) {
        m_outputDir = dir;
        pluginSettings().setValue("outputDir", dir);
        emit outputDirChanged();
    }
}

void VideoSubtitleController::setKeepWav(bool keep)
{
    pluginSettings().setValue("keepWav", keep);
    emit keepWavChanged();
}

void VideoSubtitleController::setKeepOriginalSrt(bool keep)
{
    pluginSettings().setValue("keepOriginalSrt", keep);
    emit keepOriginalSrtChanged();
}

void VideoSubtitleController::setKeepTranslatedSrt(bool keep)
{
    pluginSettings().setValue("keepTranslatedSrt", keep);
    emit keepTranslatedSrtChanged();
}

// Step control getters
bool VideoSubtitleController::enableAudioExtraction() const { return m_enableAudioExtraction; }
bool VideoSubtitleController::enableTranscribe() const { return m_enableTranscribe; }
bool VideoSubtitleController::enableTranslate() const { return m_enableTranslate; }
bool VideoSubtitleController::enableBurnSubtitle() const { return m_enableBurnSubtitle; }

// Step control setters
void VideoSubtitleController::setEnableAudioExtraction(bool enabled)
{
    if (m_enableAudioExtraction != enabled) {
        m_enableAudioExtraction = enabled;
        emit enableAudioExtractionChanged();
    }
}
void VideoSubtitleController::setEnableTranscribe(bool enabled)
{
    if (m_enableTranscribe != enabled) {
        m_enableTranscribe = enabled;
        emit enableTranscribeChanged();
    }
}
void VideoSubtitleController::setEnableTranslate(bool enabled)
{
    if (m_enableTranslate != enabled) {
        m_enableTranslate = enabled;
        emit enableTranslateChanged();
    }
}
void VideoSubtitleController::setEnableBurnSubtitle(bool enabled)
{
    if (m_enableBurnSubtitle != enabled) {
        m_enableBurnSubtitle = enabled;
        emit enableBurnSubtitleChanged();
    }
}

// Actions
void VideoSubtitleController::execute()
{
    if (m_isProcessing) return;

    if (m_inputPath.isEmpty()) {
        setStatusMessage("请先选择输入路径");
        return;
    }

    // Validate tools based on which steps are enabled
    if ((m_enableAudioExtraction || m_enableBurnSubtitle)
        && (ffmpegPath().isEmpty() || !FFmpegService::isFFmpegAvailable(ffmpegPath()))) {
        emit settingsRequired();
        setStatusMessage("请先在设置中配置 FFmpeg 路径");
        return;
    }

    if (m_enableTranscribe) {
        if (whisperPath().isEmpty() || !QFileInfo::exists(whisperPath())) {
            emit settingsRequired();
            setStatusMessage("请先在设置中配置 whisper.cpp 路径");
            return;
        }
        if (!WhisperService::isWhisperAvailable(whisperPath())) {
            emit settingsRequired();
            setStatusMessage("Whisper 运行时环境异常，请检查 whisper.dll / ggml.dll 是否齐全");
            return;
        }
        if (whisperModelPath().isEmpty() || !QFileInfo::exists(whisperModelPath())) {
            emit settingsRequired();
            setStatusMessage("请先在设置中下载 Whisper 模型");
            return;
        }
    }

    if (m_enableTranslate && translateEngine() == 0) {
        if (apiKey().isEmpty() || baiduAppId().isEmpty()) {
            emit settingsRequired();
            setStatusMessage("请先在设置中配置百度翻译 APP ID 和密钥");
            return;
        }
    }

    PluginLogger::info(QString("===== 开始批量处理 ====="));
    PluginLogger::info(QString("翻译引擎: %1, 源语言: %2, 目标语言: %3")
        .arg(translateEngine() == 0 ? "百度翻译" : "不翻译")
        .arg(m_sourceLanguage, m_targetLanguage));
    PluginLogger::info(QString("步骤: 提取音频=%1, 语音识别=%2, 翻译=%3, 烧录=%4")
        .arg(m_enableAudioExtraction ? "开" : "关")
        .arg(m_enableTranscribe ? "开" : "关")
        .arg(m_enableTranslate ? "开" : "关")
        .arg(m_enableBurnSubtitle ? "开" : "关"));

    // Collect video files
    m_pendingFiles.clear();
    m_currentFileIndex = -1;

    if (m_inputMode == 0) {
        // Single file
        if (isVideoFile(m_inputPath)) {
            m_pendingFiles.append(m_inputPath);
        } else {
            setStatusMessage("选择的文件不是支持的视频格式");
            return;
        }
    } else {
        // Directory mode
        QDir dir(m_inputPath);
        QDir::Filters filters = QDir::Files;
        if (m_recursive) {
            filters |= QDir::Dirs;
        }

        QFileInfoList entries = dir.entryInfoList(filters);
        for (const QFileInfo &info : entries) {
            if (info.isDir() && m_recursive) {
                QDir subDir(info.absoluteFilePath());
                QFileInfoList subFiles = subDir.entryInfoList(QDir::Files);
                for (const QFileInfo &subFile : subFiles) {
                    if (isVideoFile(subFile.fileName())) {
                        m_pendingFiles.append(subFile.absoluteFilePath());
                    }
                }
            } else if (info.isFile() && isVideoFile(info.fileName())) {
                m_pendingFiles.append(info.absoluteFilePath());
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
        bool useGpu = pluginSettings().value("useGpuAccel", false).toBool();
        m_ffmpegService->setUseHardwareAccel(useGpu);
        PluginLogger::info(QString("FFmpeg GPU 加速: %1").arg(useGpu ? "开启" : "关闭"));
    }

    processNextFile();
}

void VideoSubtitleController::cancel()
{
    if (!m_isProcessing) return;

    m_whisperService->cancel();
    m_translateService->cancel();
    m_ffmpegService->cancel();

    setIsProcessing(false);
    setStatusMessage("已取消处理");
    setCurrentStep("");
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
    m_inputPath.clear();
    m_pendingFiles.clear();
    m_currentFileIndex = -1;
    m_records.clear();
    setStatusMessage("");
    setCurrentStep("");
    setProgress(0.0);

    emit inputPathChanged();
    emit hasRecordsChanged();
    emit recordsChanged();
}

void VideoSubtitleController::processNextFile()
{
    m_currentFileIndex++;

    if (m_currentFileIndex >= m_pendingFiles.size()) {
        // All files done
        setIsProcessing(false);
        setStatusMessage(QString("处理完成，共 %1 个文件").arg(m_pendingFiles.size()));
        setCurrentStep("");
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
    if (m_outputMode == 1 && !m_outputDir.isEmpty()) {
        outDir = m_outputDir;
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

    PluginLogger::info(QString("开始处理视频: %1").arg(fi.fileName()));

    emit logMessage(QString("========== 开始处理: %1 ==========").arg(fi.fileName()));

    // [优化] 如果输出视频已存在，跳过整个文件
    if (m_enableBurnSubtitle && QFileInfo::exists(m_currentOutputVideoPath)) {
        QString reason = QString("输出文件已存在，跳过: %1").arg(m_currentOutputVideoPath);
        PluginLogger::info(reason);
        emit logMessage("⏭ " + fi.fileName() + " 输出已存在，跳过");
        addRecord(m_currentVideoPath, m_currentOutputVideoPath, true, "跳过（输出已存在）");
        processNextFile();
        return;
    }

    // [优化] 如果翻译字幕文件已存在，校验格式和结束时间后决定是否直接跳转到烧录
    // （去重后字幕总时长可能不足视频一半，所以不再要求 ≥50% 覆盖）
    if (m_enableBurnSubtitle && QFileInfo::exists(m_currentTranslatedSrtPath)) {
        QList<SubtitleService::SubtitleEntry> existingEntries =
            SubtitleService::parseSrt(m_currentTranslatedSrtPath);
        if (!existingEntries.isEmpty()) {
            qint64 srtLastEnd = existingEntries.last().endTime;
            qint64 videoDuration = FFmpegService::getVideoDuration(ffmpegPath(),
                                                                    m_currentVideoPath);
            bool timeValid = (videoDuration <= 0)
                          || (srtLastEnd <= videoDuration + 2000); // 结束时间不超出视频2秒以上
            if (timeValid) {
                PluginLogger::info(
                    QString("检测到已有翻译字幕 (%1 条字幕)，校验通过，直接烧录")
                        .arg(existingEntries.size()));
                emit logMessage("✓ 检测到已有翻译字幕，跳过前置步骤，直接烧录");

                setCurrentStep("烧录字幕");
                setProgress(0.0);
                disconnect(m_ffmpegService, &FFmpegService::finished, nullptr, nullptr);
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
                PluginLogger::info(
                    QString("已有翻译字幕结束时间(%1 ms)超出视频时长(%2 ms)，重新处理")
                        .arg(srtLastEnd).arg(videoDuration));
                emit logMessage("⚠ 已有翻译字幕结束时间超出视频范围，重新处理");
            }
        } else {
            PluginLogger::info("已有翻译字幕文件为空或格式错误，重新处理: "
                               + m_currentTranslatedSrtPath);
            emit logMessage("⚠ 已有翻译字幕无效，重新处理");
        }
    }

    // [优化] 如果音频文件已存在且有效，跳过音频提取
    if (m_enableAudioExtraction && QFileInfo::exists(m_currentAudioPath)) {
        qint64 wavSize = QFileInfo(m_currentAudioPath).size();
        if (wavSize > 1024) {  // 有效音频文件至少 1KB
            PluginLogger::info(QString("检测到已有音频文件 (%1 bytes)，跳过提取").arg(wavSize));
            emit logMessage("✓ 检测到已有音频文件，跳过提取");
            disconnect(m_ffmpegService, &FFmpegService::finished, nullptr, nullptr);
            connect(m_ffmpegService, &FFmpegService::finished,
                    this, &VideoSubtitleController::onAudioExtracted);
            onAudioExtracted(true, m_currentAudioPath, "");
            return;
        } else {
            PluginLogger::info(QString("已有音频文件过小 (%1 bytes)，重新提取").arg(wavSize));
            emit logMessage("⚠ 已有音频文件过小，重新提取");
        }
    }

    if (m_enableAudioExtraction) {
        // Step 1: Extract audio
        setCurrentStep("提取音频");
        setProgress(0.0);
        PluginLogger::info("步骤 1/4: 提取音频 → " + m_currentAudioPath);
        disconnect(m_ffmpegService, &FFmpegService::finished, nullptr, nullptr);
        connect(m_ffmpegService, &FFmpegService::finished,
                this, &VideoSubtitleController::onAudioExtracted);
        m_ffmpegService->startExtractAudio(ffmpegPath(), videoPath, m_currentAudioPath);
        emit logMessage("音频提取...");
    } else {
        PluginLogger::info("步骤 1/4: 跳过提取音频");
        emit logMessage("跳过音频提取");
        // Skip audio extraction, proceed to next enabled step
        onAudioExtracted(true, m_currentAudioPath, "");
    }
}

void VideoSubtitleController::onAudioExtracted(bool success, const QString &audioPath, const QString &error)
{
    if (!success) {
        PluginLogger::error("提取音频失败: " + error);
        emit logMessage("✗ 音频提取失败: " + error);
        addRecord(m_currentVideoPath, "", false, "提取音频失败: " + error);
        processNextFile();
        return;
    }

    PluginLogger::info("音频提取完成: " + audioPath);
    emit logMessage("✓ 音频提取完成");

    if (m_enableTranscribe) {
        // [优化] 如果原版字幕文件已存在且有效，跳过语音识别
        if (QFileInfo::exists(m_currentOriginalSrtPath)) {
            QList<SubtitleService::SubtitleEntry> existingEntries =
                SubtitleService::parseSrt(m_currentOriginalSrtPath);
            if (!existingEntries.isEmpty()) {
                PluginLogger::info(QString("检测到已有原版字幕 (%1 条字幕)，跳过语音识别")
                    .arg(existingEntries.size()));
                emit logMessage("✓ 检测到已有原版字幕，跳过语音识别");
                onTranscribeFinished(true, m_currentOriginalSrtPath, "");
                return;
            } else {
                PluginLogger::info("已有原版字幕文件为空或格式错误，重新识别: "
                                   + m_currentOriginalSrtPath);
                emit logMessage("⚠ 已有原版字幕无效，重新识别");
            }
        }

        // Step 2: Transcribe
        setCurrentStep("语音识别");
        setProgress(0.0);
        emit logMessage("语音识别开始...");
        PluginLogger::info("步骤 2/4: 语音识别中...");
        QFileInfo audioInfo(m_currentAudioPath);
        QString outputDir = audioInfo.absolutePath();
        // Pass audioSegmentDuration for virtual segment progress display
        m_whisperService->startTranscribe(whisperPath(), whisperModelPath(),
                                           audioPath, outputDir, m_sourceLanguage,
                                           audioSegmentDuration());
    } else {
        PluginLogger::info("步骤 2/4: 跳过语音识别");
        emit logMessage("跳过语音识别");
        // Skip transcribe, proceed to next enabled step
        onTranscribeFinished(true, m_currentOriginalSrtPath, "");
    }
}

void VideoSubtitleController::onTranscribeFinished(bool success, const QString &srtPath, const QString &error)
{
    if (!success) {
        PluginLogger::error("语音识别失败: " + error);
        emit logMessage("✗ 语音识别失败: " + error);
        addRecord(m_currentVideoPath, "", false, "语音识别失败: " + error);
        processNextFile();
        return;
    }

    m_currentOriginalSrtPath = srtPath;
    PluginLogger::info("语音识别完成，SRT: " + srtPath);
    emit logMessage("✓ 语音识别完成");

    // [优化] 去重处理：移除连续重复的字幕文本后再发起翻译
    {
        QList<SubtitleService::SubtitleEntry> entries =
            SubtitleService::parseSrt(m_currentOriginalSrtPath);
        if (!entries.isEmpty()) {
            int beforeCount = entries.size();
            QList<SubtitleService::SubtitleEntry> deduped =
                SubtitleService::deduplicate(entries);
            int removedCount = beforeCount - deduped.size();
            if (removedCount > 0) {
                PluginLogger::info(
                    QString("去重: 移除 %1 条连续重复字幕（共 %2 → %3 条）")
                        .arg(removedCount).arg(beforeCount).arg(deduped.size()));
                emit logMessage(QString("✓ 去重完成: 移除 %1 条重复字幕（%2 → %3）")
                    .arg(removedCount).arg(beforeCount).arg(deduped.size()));
                SubtitleService::writeSrt(m_currentOriginalSrtPath, deduped);
            } else {
                PluginLogger::info("字幕无连续重复，无需去重");
            }
        }
    }

    // Step 3: Translate
    if (m_enableTranslate) {
        setCurrentStep("翻译字幕");
        setProgress(0.0);
        emit logMessage("翻译字幕...");
        PluginLogger::info("步骤 3/4: 翻译字幕中...");
        m_translateService->startTranslate(m_currentOriginalSrtPath,
                                            m_currentTranslatedSrtPath,
                                            translateEngine(), apiKey(),
                                            apiUrl(), m_targetLanguage,
                                            baiduAppId());
    } else {
        PluginLogger::info("步骤 3/4: 跳过翻译");
        emit logMessage("跳过翻译");
        // No translation, treat original as translated and skip to burning
        m_currentTranslatedSrtPath = m_currentOriginalSrtPath;
        onTranslateFinished(true, m_currentOriginalSrtPath, "");
    }
}

void VideoSubtitleController::onTranslateFinished(bool success, const QString &srtPath, const QString &error)
{
    if (!success) {
        PluginLogger::error("翻译失败: " + error);
        emit logMessage("✗ 翻译失败: " + error);
        addRecord(m_currentVideoPath, "", false, "翻译失败: " + error);
        processNextFile();
        return;
    }

    m_currentTranslatedSrtPath = srtPath;
    PluginLogger::info("翻译完成，SRT: " + srtPath);
    emit logMessage("✓ 翻译完成");

    // Step 4: Burn subtitles
    if (m_enableBurnSubtitle) {
        setCurrentStep("烧录字幕");
        setProgress(0.0);
        emit logMessage("烧录字幕...");
        PluginLogger::info("步骤 4/4: 烧录字幕中...");
        disconnect(m_ffmpegService, &FFmpegService::finished, nullptr, nullptr);
        connect(m_ffmpegService, &FFmpegService::finished,
                this, &VideoSubtitleController::onBurnFinished);
        m_ffmpegService->startBurnSubtitles(ffmpegPath(), m_currentVideoPath,
                                             m_currentTranslatedSrtPath,
                                             m_currentOutputVideoPath,
                                             defaultFontSize(),
                                             defaultFontColor(),
                                             defaultBorderColor(),
                                             defaultBorderWidth());
    } else {
        PluginLogger::info("步骤 4/4: 跳过烧录字幕");
        emit logMessage("跳过烧录字幕");
        // No burn, finalize with SRT output
        onBurnFinished(true, m_currentTranslatedSrtPath, "");
    }
}

void VideoSubtitleController::onBurnFinished(bool success, const QString &outputPath, const QString &error)
{
    if (success) {
        PluginLogger::info(QString("烧录完成: %1 → %2").arg(m_currentVideoPath, outputPath));
        emit logMessage("✓ 烧录完成");
    } else {
        PluginLogger::error("烧录字幕失败: " + error);
        emit logMessage("✗ 烧录字幕失败: " + error);
    }
    finalizeCurrentFile();
}

void VideoSubtitleController::finalizeCurrentFile()
{
    QFileInfo fi(m_currentVideoPath);
    QString baseName = fi.completeBaseName();
    QString outDir = (m_outputMode == 1 && !m_outputDir.isEmpty())
                         ? m_outputDir
                         : fi.absolutePath();
    QString intermediateDir = outDir + "/" + baseName;

    // Track the primary output for the record
    QString outputPath;
    bool hasAnyOutput = false;

    // WAV — keep or discard
    if (m_enableAudioExtraction && QFileInfo::exists(m_currentAudioPath)) {
        if (keepWav()) {
            outputPath = m_currentAudioPath;
            PluginLogger::info("输出音频: " + m_currentAudioPath);
            emit logMessage("✓ 生成音频: " + m_currentAudioPath);
            hasAnyOutput = true;
        } else {
            QFile::remove(m_currentAudioPath);
            PluginLogger::info("丢弃音频: " + m_currentAudioPath);
        }
    }

    // Original SRT — keep or discard
    if (m_enableTranscribe && QFileInfo::exists(m_currentOriginalSrtPath)) {
        if (keepOriginalSrt()) {
            outputPath = m_currentOriginalSrtPath;
            PluginLogger::info("输出字幕: " + m_currentOriginalSrtPath);
            emit logMessage("✓ 生成字幕: " + m_currentOriginalSrtPath);
            hasAnyOutput = true;
        } else {
            QFile::remove(m_currentOriginalSrtPath);
            PluginLogger::info("丢弃字幕: " + m_currentOriginalSrtPath);
        }
    }

    // Translated SRT — keep or discard
    if (m_enableTranslate && QFileInfo::exists(m_currentTranslatedSrtPath)) {
        if (keepTranslatedSrt()) {
            outputPath = m_currentTranslatedSrtPath;
            PluginLogger::info("输出翻译字幕: " + m_currentTranslatedSrtPath);
            emit logMessage("✓ 生成翻译字幕: " + m_currentTranslatedSrtPath);
            hasAnyOutput = true;
        } else {
            QFile::remove(m_currentTranslatedSrtPath);
            PluginLogger::info("丢弃翻译字幕: " + m_currentTranslatedSrtPath);
        }
    }

    // Burn output video is always kept (main output)
    if (m_enableBurnSubtitle && QFileInfo::exists(m_currentOutputVideoPath)) {
        outputPath = m_currentOutputVideoPath;
        PluginLogger::info("输出视频: " + outputPath);
        emit logMessage("✓ 生成视频: " + outputPath);
        hasAnyOutput = true;
    }

    // Record result
    if (hasAnyOutput && QFileInfo::exists(outputPath)) {
        addRecord(m_currentVideoPath, outputPath, true, "已完成");
        emit logMessage(QString("========== 处理完成: %1 ==========").arg(outputPath));
    } else if (hasAnyOutput) {
        addRecord(m_currentVideoPath, outputPath, true, "已完成（中间文件）");
        emit logMessage(QString("========== 处理完成: %1 ==========").arg(outputPath));
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
    // Calculate how many steps are before transcribe and how many total
    int stepsBefore = m_enableAudioExtraction ? 1 : 0;
    int totalSteps = (m_enableAudioExtraction ? 1 : 0)
                   + (m_enableTranscribe ? 1 : 0)
                   + (m_enableTranslate ? 1 : 0)
                   + (m_enableBurnSubtitle ? 1 : 0);
    if (totalSteps == 0) totalSteps = 1;
    double stepWeight = 1.0 / totalSteps;
    setProgress(stepsBefore * stepWeight + value * stepWeight);
}

void VideoSubtitleController::onTranslateProgress(double value)
{
    int stepsBefore = (m_enableAudioExtraction ? 1 : 0)
                    + (m_enableTranscribe ? 1 : 0);
    int totalSteps = (m_enableAudioExtraction ? 1 : 0)
                   + (m_enableTranscribe ? 1 : 0)
                   + (m_enableTranslate ? 1 : 0)
                   + (m_enableBurnSubtitle ? 1 : 0);
    if (totalSteps == 0) totalSteps = 1;
    double stepWeight = 1.0 / totalSteps;
    setProgress(stepsBefore * stepWeight + value * stepWeight);
}

void VideoSubtitleController::onFFmpegProgress(double value)
{
    double stepWeight = 1.0;
    int totalSteps = (m_enableAudioExtraction ? 1 : 0)
                   + (m_enableTranscribe ? 1 : 0)
                   + (m_enableTranslate ? 1 : 0)
                   + (m_enableBurnSubtitle ? 1 : 0);
    if (totalSteps == 0) totalSteps = 1;
    stepWeight = 1.0 / totalSteps;

    if (m_currentStep == "提取音频") {
        setProgress(0.0 + value * stepWeight);
    } else if (m_currentStep == "烧录字幕") {
        int stepsBefore = (m_enableAudioExtraction ? 1 : 0)
                        + (m_enableTranscribe ? 1 : 0)
                        + (m_enableTranslate ? 1 : 0);
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

void VideoSubtitleController::setCurrentStep(const QString &step)
{
    if (m_currentStep != step) {
        m_currentStep = step;
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

bool VideoSubtitleController::isVideoFile(const QString &fileName) const
{
    QString lower = fileName.toLower();
    for (const QString &ext : m_videoExtensions) {
        if (lower.endsWith(ext)) return true;
    }
    return false;
}

QString VideoSubtitleController::ffmpegPath() const
{
    QSettings &s = pluginSettings();
    s.sync();  // 重新读取 config.ini，确保设置页面保存的值已生效
    return s.value("ffmpegPath").toString();
}

QString VideoSubtitleController::whisperPath() const
{
    QSettings &s = pluginSettings();
    s.sync();
    return s.value("whisperPath").toString();
}

QString VideoSubtitleController::whisperModelPath() const
{
    QSettings &s = pluginSettings();
    s.sync();

    // 1. Check if user specified a local model file
    QString localPath = s.value("localModelPath").toString();
    if (!localPath.isEmpty() && QFileInfo::exists(localPath)) {
        return localPath;
    }

    // 2. Use downloaded model
    int model = s.value("whisperModel", 3).toInt();
    QString modelDir = s.value("whisperModelDir",
        QCoreApplication::applicationDirPath() + "/plugins/videosubtitle").toString();

    QStringList modelFiles = {"ggml-tiny.bin", "ggml-base.bin", "ggml-small.bin", "ggml-medium.bin", "ggml-large-v3.bin"};
    if (model >= 0 && model < modelFiles.size()) {
        return modelDir + "/" + modelFiles[model];
    }
    return QString();
}

QString VideoSubtitleController::apiKey() const
{
    return QByteArray::fromBase64(pluginSettings().value("apiKey").toByteArray());
}

QString VideoSubtitleController::apiUrl() const
{
    return pluginSettings().value("apiUrl").toString();
}

QString VideoSubtitleController::baiduAppId() const
{
    QSettings &s = pluginSettings();
    s.sync();
    return s.value("baiduAppId").toString();
}

int VideoSubtitleController::audioSegmentDuration() const
{
    QSettings &s = pluginSettings();
    s.sync();
    return s.value("audioSegmentDuration", 10).toInt();
}

int VideoSubtitleController::translateEngine() const
{
    QSettings &s = pluginSettings();
    s.sync();
    return s.value("translateEngine", 0).toInt();
}

int VideoSubtitleController::defaultFontSize() const
{
    QSettings &s = pluginSettings();
    s.sync();
    return s.value("defaultFontSize", 20).toInt();
}

QString VideoSubtitleController::defaultFontColor() const
{
    QSettings &s = pluginSettings();
    s.sync();
    return s.value("defaultFontColor", "#FFFFFF").toString();
}

QString VideoSubtitleController::defaultBorderColor() const
{
    QSettings &s = pluginSettings();
    s.sync();
    return s.value("defaultBorderColor", "#000000").toString();
}

int VideoSubtitleController::defaultBorderWidth() const
{
    QSettings &s = pluginSettings();
    s.sync();
    return s.value("defaultBorderWidth", 2).toInt();
}
