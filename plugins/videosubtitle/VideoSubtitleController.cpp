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
    connect(m_translateService, &TranslateService::progress,
            this, &VideoSubtitleController::onTranslateProgress);
    connect(m_translateService, &TranslateService::finished,
            this, &VideoSubtitleController::onTranslateFinished);
    connect(m_ffmpegService, &FFmpegService::progress,
            this, &VideoSubtitleController::onFFmpegProgress);
    // NOTE: FFmpegService::finished is NOT connected here permanently.
    // It is dynamically connected to onAudioExtracted or onBurnFinished
    // depending on the current step (see processSingleFile / onTranslateFinished).
}

// Getters
QString VideoSubtitleController::inputPath() const { return m_inputPath; }
int VideoSubtitleController::inputMode() const { return m_inputMode; }
bool VideoSubtitleController::recursive() const { return m_recursive; }
QString VideoSubtitleController::sourceLanguage() const { return m_sourceLanguage; }
QString VideoSubtitleController::targetLanguage() const { return m_targetLanguage; }
int VideoSubtitleController::subtitleStyle() const { return m_subtitleStyle; }
bool VideoSubtitleController::bilingual() const { return m_bilingual; }
int VideoSubtitleController::outputMode() const { return m_outputMode; }
QString VideoSubtitleController::outputDir() const { return m_outputDir; }
bool VideoSubtitleController::keepOriginalSrt() const { return m_keepOriginalSrt; }
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
    if (m_subtitleStyle != style) {
        m_subtitleStyle = style;
        emit subtitleStyleChanged();
    }
}

void VideoSubtitleController::setBilingual(bool bilingual)
{
    if (m_bilingual != bilingual) {
        m_bilingual = bilingual;
        emit bilingualChanged();
    }
}

void VideoSubtitleController::setOutputMode(int mode)
{
    if (m_outputMode != mode) {
        m_outputMode = mode;
        emit outputModeChanged();
    }
}

void VideoSubtitleController::setOutputDir(const QString &dir)
{
    if (m_outputDir != dir) {
        m_outputDir = dir;
        emit outputDirChanged();
    }
}

void VideoSubtitleController::setKeepOriginalSrt(bool keep)
{
    if (m_keepOriginalSrt != keep) {
        m_keepOriginalSrt = keep;
        emit keepOriginalSrtChanged();
    }
}

// Actions
void VideoSubtitleController::execute()
{
    if (m_isProcessing) return;

    // Check tools are available
    if (ffmpegPath().isEmpty() || !FFmpegService::isFFmpegAvailable(ffmpegPath())) {
        emit settingsRequired();
        setStatusMessage("请先在设置中配置 FFmpeg 路径");
        return;
    }

    if (whisperPath().isEmpty() || !QFileInfo::exists(whisperPath())) {
        emit settingsRequired();
        setStatusMessage("请先在设置中配置 whisper.cpp 路径");
        return;
    }

    if (whisperModelPath().isEmpty() || !QFileInfo::exists(whisperModelPath())) {
        emit settingsRequired();
        setStatusMessage("请先在设置中下载 Whisper 模型");
        return;
    }

    if (translateEngine() == 0 && apiKey().isEmpty()) {
        emit settingsRequired();
        setStatusMessage("请先在设置中配置翻译 API Key");
        return;
    }

    if (m_inputPath.isEmpty()) {
        setStatusMessage("请先选择输入路径");
        return;
    }

    PluginLogger::info(QString("===== 开始批量处理 ====="));
    PluginLogger::info(QString("翻译引擎: %1, 源语言: %2, 目标语言: %3, 双语: %4")
        .arg(translateEngine() == 0 ? "百度翻译" : "不翻译")
        .arg(m_sourceLanguage, m_targetLanguage)
        .arg(m_bilingual ? "是" : "否"));
    PluginLogger::info(QString("FFmpeg: %1, Whisper: %2, 模型: %3")
        .arg(ffmpegPath(), whisperPath(), whisperModelPath()));

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

    // Create temp directory for processing
    QString tempDir = outDir + "/.video_subtitle_temp";
    QDir().mkpath(tempDir);

    m_currentAudioPath = tempDir + "/" + baseName + ".wav";
    m_currentOriginalSrtPath = tempDir + "/" + baseName + ".srt";
    m_currentTranslatedSrtPath = outDir + "/" + baseName + "_translated.srt";
    m_currentOutputVideoPath = outDir + "/" + baseName + "_subtitled" + fi.suffix();

    setStatusMessage(QString("处理中 [%1/%2]: %3")
        .arg(m_currentFileIndex + 1)
        .arg(m_pendingFiles.size())
        .arg(fi.fileName()));

    // Step 1: Extract audio
    setCurrentStep("提取音频");
    setProgress(0.0);
    PluginLogger::info(QString("开始处理视频: %1").arg(fi.fileName()));
    PluginLogger::info("步骤 1/4: 提取音频 → " + m_currentAudioPath);
    // Route FFmpeg finished → onAudioExtracted
    disconnect(m_ffmpegService, &FFmpegService::finished, nullptr, nullptr);
    connect(m_ffmpegService, &FFmpegService::finished,
            this, &VideoSubtitleController::onAudioExtracted);
    m_ffmpegService->startExtractAudio(ffmpegPath(), videoPath, m_currentAudioPath);
}

void VideoSubtitleController::onAudioExtracted(bool success, const QString &audioPath, const QString &error)
{
    if (!success) {
        PluginLogger::error("提取音频失败: " + error);
        addRecord(m_currentVideoPath, "", false, "提取音频失败: " + error);
        processNextFile();
        return;
    }

    PluginLogger::info("音频提取完成: " + audioPath);

    // Step 2: Transcribe
    setCurrentStep("语音识别");
    setProgress(0.0);
    PluginLogger::info("步骤 2/4: 语音识别中...");
    QFileInfo audioInfo(m_currentAudioPath);
    QString outputDir = audioInfo.absolutePath();
    m_whisperService->startTranscribe(whisperPath(), whisperModelPath(),
                                       audioPath, outputDir, m_sourceLanguage);
}

void VideoSubtitleController::onTranscribeFinished(bool success, const QString &srtPath, const QString &error)
{
    if (!success) {
        PluginLogger::error("语音识别失败: " + error);
        addRecord(m_currentVideoPath, "", false, "语音识别失败: " + error);
        processNextFile();
        return;
    }

    m_currentOriginalSrtPath = srtPath;
    PluginLogger::info("语音识别完成，SRT: " + srtPath);

    // Step 3: Translate
    if (translateEngine() == 3) {
        PluginLogger::info("步骤 3/4: 跳过翻译（不翻译模式）");
        // No translation, treat original as translated and skip to burning
        m_currentTranslatedSrtPath = m_currentOriginalSrtPath;
        onTranslateFinished(true, m_currentOriginalSrtPath, "");
        return;
    }

    setCurrentStep("翻译字幕");
    setProgress(0.0);
    PluginLogger::info("步骤 3/4: 翻译字幕中...");
    m_translateService->startTranslate(m_currentOriginalSrtPath,
                                        m_currentTranslatedSrtPath,
                                        translateEngine(), apiKey(),
                                        apiUrl(), m_targetLanguage,
                                        baiduAppId(),
                                        m_bilingual);
}

void VideoSubtitleController::onTranslateFinished(bool success, const QString &srtPath, const QString &error)
{
    if (!success) {
        PluginLogger::error("翻译失败: " + error);
        addRecord(m_currentVideoPath, "", false, "翻译失败: " + error);
        processNextFile();
        return;
    }

    m_currentTranslatedSrtPath = srtPath;
    PluginLogger::info("翻译完成，SRT: " + srtPath);

    // Step 4: Burn subtitles
    setCurrentStep("烧录字幕");
    setProgress(0.0);
    PluginLogger::info("步骤 4/4: 烧录字幕中...");
    // Route FFmpeg finished → onBurnFinished
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
}

void VideoSubtitleController::onBurnFinished(bool success, const QString &outputPath, const QString &error)
{
    QFileInfo fi(m_currentVideoPath);
    QString baseName = fi.completeBaseName();
    QString outDir = (m_outputMode == 1 && !m_outputDir.isEmpty())
                         ? m_outputDir
                         : fi.absolutePath();

    if (success) {
        PluginLogger::info(QString("烧录完成: %1 → %2").arg(m_currentVideoPath, outputPath));
        // Keep original SRT files if requested
        if (m_keepOriginalSrt) {
            QString tempDirPath = QFileInfo(m_currentAudioPath).absolutePath();
            QDir tempDir(tempDirPath);
            if (tempDir.exists()) {
                QStringList srtFiles = tempDir.entryList({"*.srt"}, QDir::Files);
                for (const QString &srtFile : srtFiles) {
                    QString srcPath = tempDir.filePath(srtFile);
                    QString dstPath = outDir + "/" + srtFile;
                    // Avoid overwriting the translated SRT which is already in outDir
                    if (QFileInfo(dstPath) != QFileInfo(srcPath)) {
                        QFile::copy(srcPath, dstPath);
                        PluginLogger::info("保留字幕文件: " + dstPath);
                    }
                }
            }
        }

        addRecord(m_currentVideoPath, outputPath, true, "已完成");
    } else {
        PluginLogger::error("烧录字幕失败: " + error);
        addRecord(m_currentVideoPath, "", false, "烧录字幕失败: " + error);
    }

    // Clean up temp directory
    QString tempDirPath = QFileInfo(m_currentAudioPath).absolutePath();
    QDir tempDir(tempDirPath);
    if (tempDir.exists() && tempDir.dirName() == ".video_subtitle_temp") {
        tempDir.removeRecursively();
    }

    processNextFile();
}

void VideoSubtitleController::onWhisperProgress(double value)
{
    // Whisper progress is 0-1, map to overall progress
    // This is step 2 of 4, so progress is 25% + value * 25%
    setProgress(0.25 + value * 0.25);
}

void VideoSubtitleController::onTranslateProgress(double value)
{
    setProgress(0.50 + value * 0.25);
}

void VideoSubtitleController::onFFmpegProgress(double value)
{
    if (m_currentStep == "提取音频") {
        setProgress(value * 0.25);
    } else if (m_currentStep == "烧录字幕") {
        setProgress(0.75 + value * 0.25);
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
        QCoreApplication::applicationDirPath() + "/../plugins/videosubtitle/models").toString();

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
    return QSettings("BYTools", "VideoSubtitle").value("baiduAppId").toString();
}

int VideoSubtitleController::translateEngine() const
{
    return QSettings("BYTools", "VideoSubtitle").value("translateEngine", 0).toInt();
}

int VideoSubtitleController::defaultFontSize() const
{
    return QSettings("BYTools", "VideoSubtitle").value("defaultFontSize", 20).toInt();
}

QString VideoSubtitleController::defaultFontColor() const
{
    return QSettings("BYTools", "VideoSubtitle").value("defaultFontColor", "#FFFFFF").toString();
}

QString VideoSubtitleController::defaultBorderColor() const
{
    return QSettings("BYTools", "VideoSubtitle").value("defaultBorderColor", "#000000").toString();
}

int VideoSubtitleController::defaultBorderWidth() const
{
    return QSettings("BYTools", "VideoSubtitle").value("defaultBorderWidth", 2).toInt();
}
