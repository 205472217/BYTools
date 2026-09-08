#pragma once

#include <QObject>
#include <QVariantList>
#include <QThread>

class PluginLogger;
class WhisperService;
class TranslateService;
class FFmpegService;
class VideoSubtitleSettings;

class VideoSubtitleController : public QObject
{
    Q_OBJECT
    // === Input settings ===
    Q_PROPERTY(QString inputPath READ inputPath WRITE setInputPath NOTIFY inputPathChanged)
    Q_PROPERTY(int inputMode READ inputMode WRITE setInputMode NOTIFY inputModeChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)

    // === Language settings ===
    Q_PROPERTY(QString sourceLanguage READ sourceLanguage WRITE setSourceLanguage NOTIFY sourceLanguageChanged)
    Q_PROPERTY(QString targetLanguage READ targetLanguage WRITE setTargetLanguage NOTIFY targetLanguageChanged)
    Q_PROPERTY(bool translateMusic READ translateMusic WRITE setTranslateMusic NOTIFY translateMusicChanged)

    // === Subtitle style ===
    Q_PROPERTY(int subtitleStyle READ subtitleStyle WRITE setSubtitleStyle NOTIFY subtitleStyleChanged)

    // === Output settings ===
    Q_PROPERTY(int outputMode READ outputMode WRITE setOutputMode NOTIFY outputModeChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool keepWav READ keepWav WRITE setKeepWav NOTIFY keepWavChanged)
    Q_PROPERTY(bool keepOriginalSrt READ keepOriginalSrt WRITE setKeepOriginalSrt NOTIFY keepOriginalSrtChanged)
    Q_PROPERTY(bool keepTranslatedSrt READ keepTranslatedSrt WRITE setKeepTranslatedSrt NOTIFY keepTranslatedSrtChanged)

    // === Step control ===
    Q_PROPERTY(bool enableAudioExtraction READ enableAudioExtraction WRITE setEnableAudioExtraction NOTIFY enableAudioExtractionChanged)
    Q_PROPERTY(bool enableTranscribe READ enableTranscribe WRITE setEnableTranscribe NOTIFY enableTranscribeChanged)
    Q_PROPERTY(bool enableTranslate READ enableTranslate WRITE setEnableTranslate NOTIFY enableTranslateChanged)
    Q_PROPERTY(bool enableBurnSubtitle READ enableBurnSubtitle WRITE setEnableBurnSubtitle NOTIFY enableBurnSubtitleChanged)

    // === Status ===
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)
    Q_PROPERTY(int currentStep READ currentStep NOTIFY currentStepChanged)
    Q_PROPERTY(QString currentStepName READ currentStepName NOTIFY currentStepChanged)

    // === Step enum constants for QML ===
    Q_PROPERTY(int stepNone READ stepNone CONSTANT)
    Q_PROPERTY(int stepExtractAudio READ stepExtractAudio CONSTANT)
    Q_PROPERTY(int stepTranscribe READ stepTranscribe CONSTANT)
    Q_PROPERTY(int stepTranslate READ stepTranslate CONSTANT)
    Q_PROPERTY(int stepBurnSubtitle READ stepBurnSubtitle CONSTANT)

    // === Tool paths ===
    Q_PROPERTY(QString ffmpegPath READ ffmpegPath WRITE setFfmpegPath NOTIFY ffmpegPathChanged)
    Q_PROPERTY(QString ffmpegStatus READ ffmpegStatus NOTIFY ffmpegStatusChanged)
    Q_PROPERTY(bool ffmpegDetecting READ ffmpegDetecting NOTIFY ffmpegDetectingChanged)
    Q_PROPERTY(QString whisperPath READ whisperPath WRITE setWhisperPath NOTIFY whisperPathChanged)
    Q_PROPERTY(QString whisperStatus READ whisperStatus NOTIFY whisperStatusChanged)
    Q_PROPERTY(bool whisperDetecting READ whisperDetecting NOTIFY whisperDetectingChanged)
    Q_PROPERTY(int whisperModel READ whisperModel WRITE setWhisperModel NOTIFY whisperModelChanged)
    Q_PROPERTY(QString whisperModelDir READ whisperModelDir WRITE setWhisperModelDir NOTIFY whisperModelDirChanged)
    Q_PROPERTY(QVariantList availableModels READ availableModels NOTIFY availableModelsChanged)
    Q_PROPERTY(QString localModelPath READ localModelPath WRITE setLocalModelPath NOTIFY localModelPathChanged)
    Q_PROPERTY(int audioSegmentDuration READ audioSegmentDuration WRITE setAudioSegmentDuration NOTIFY audioSegmentDurationChanged)

    // === Translate engine ===
    Q_PROPERTY(int translateEngine READ translateEngine WRITE setTranslateEngine NOTIFY translateEngineChanged)
    Q_PROPERTY(QStringList translateEngineNames READ translateEngineNames CONSTANT)
    Q_PROPERTY(QString baiduAppId READ baiduAppId WRITE setBaiduAppId NOTIFY baiduAppIdChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)
    Q_PROPERTY(QString apiTestResult READ apiTestResult NOTIFY apiTestResultChanged)
    Q_PROPERTY(bool apiTesting READ apiTesting NOTIFY apiTestingChanged)
    Q_PROPERTY(QString libreTranslateUrl READ libreTranslateUrl WRITE setLibreTranslateUrl NOTIFY libreTranslateUrlChanged)
    Q_PROPERTY(QString libreTranslateStatus READ libreTranslateStatus NOTIFY libreTranslateStatusChanged)

    // === Subtitle style ===
    Q_PROPERTY(int defaultFontSize READ defaultFontSize WRITE setDefaultFontSize NOTIFY defaultFontSizeChanged)
    Q_PROPERTY(QString defaultFontColor READ defaultFontColor WRITE setDefaultFontColor NOTIFY defaultFontColorChanged)
    Q_PROPERTY(QString defaultBorderColor READ defaultBorderColor WRITE setDefaultBorderColor NOTIFY defaultBorderColorChanged)
    Q_PROPERTY(int defaultBorderWidth READ defaultBorderWidth WRITE setDefaultBorderWidth NOTIFY defaultBorderWidthChanged)

    // === GPU & output ===
    Q_PROPERTY(bool useGpuAccel READ useGpuAccel WRITE setUseGpuAccel NOTIFY useGpuAccelChanged)
    Q_PROPERTY(int quality READ quality WRITE setQuality NOTIFY qualityChanged)
    Q_PROPERTY(QString gpuAccelInfo READ gpuAccelInfo NOTIFY gpuAccelInfoChanged)

public:
    enum Step { StepNone = 0, StepExtractAudio, StepTranscribe, StepTranslate, StepBurnSubtitle };
    explicit VideoSubtitleController(PluginLogger *logger, VideoSubtitleSettings *settings, QObject *parent = nullptr);
    ~VideoSubtitleController() override;

    // Getters
    QString inputPath() const;
    int inputMode() const;
    bool recursive() const;
    QString sourceLanguage() const;
    QString targetLanguage() const;
    bool translateMusic() const;
    int subtitleStyle() const;
    int outputMode() const;
    QString outputDir() const;
    bool keepWav() const;
    bool keepOriginalSrt() const;
    bool keepTranslatedSrt() const;
    QString statusMessage() const;
    double progress() const;
    bool isProcessing() const;
    bool hasRecords() const;
    QVariantList records() const;
    int currentStep() const;
    QString currentStepName() const;

    // Step enum read-only access for QML
    int stepNone() const { return StepNone; }
    int stepExtractAudio() const { return StepExtractAudio; }
    int stepTranscribe() const { return StepTranscribe; }
    int stepTranslate() const { return StepTranslate; }
    int stepBurnSubtitle() const { return StepBurnSubtitle; }

    // === Tool paths getters ===
    QString ffmpegPath() const;
    QString ffmpegStatus() const;
    bool ffmpegDetecting() const;
    QString whisperPath() const;
    QString whisperStatus() const;
    bool whisperDetecting() const;
    int whisperModel() const;
    QString whisperModelDir() const;
    QVariantList availableModels() const;
    QString localModelPath() const;
    int audioSegmentDuration() const;

    // === Translate engine getters ===
    int translateEngine() const;
    QStringList translateEngineNames() const;
    QString baiduAppId() const;
    QString apiKey() const;
    QString apiUrl() const;
    QString apiTestResult() const;
    bool apiTesting() const;
    QString libreTranslateUrl() const;
    QString libreTranslateStatus() const;

    // === Subtitle style getters ===
    int defaultFontSize() const;
    QString defaultFontColor() const;
    QString defaultBorderColor() const;
    int defaultBorderWidth() const;

    // === GPU & output getters ===
    bool useGpuAccel() const;
    int quality() const;
    QString gpuAccelInfo() const;

    // Setters
    void setInputPath(const QString &path);
    void setInputMode(int mode);
    void setRecursive(bool recursive);
    void setSourceLanguage(const QString &lang);
    void setTargetLanguage(const QString &lang);
    void setTranslateMusic(bool enabled);
    void setSubtitleStyle(int style);
    void setOutputMode(int mode);
    void setOutputDir(const QString &dir);
    void setKeepWav(bool keep);
    void setKeepOriginalSrt(bool keep);
    void setKeepTranslatedSrt(bool keep);

    // Step control getters
    bool enableAudioExtraction() const;
    bool enableTranscribe() const;
    bool enableTranslate() const;
    bool enableBurnSubtitle() const;

    // Step control setters
    void setEnableAudioExtraction(bool enabled);
    void setEnableTranscribe(bool enabled);
    void setEnableTranslate(bool enabled);
    void setEnableBurnSubtitle(bool enabled);

    // === Tool paths setters ===
    void setFfmpegPath(const QString &path);
    void setWhisperPath(const QString &path);
    void setWhisperModel(int model);
    void setWhisperModelDir(const QString &path);
    void setLocalModelPath(const QString &path);
    void setAudioSegmentDuration(int seconds);

    // === Translate engine setters ===
    void setTranslateEngine(int engine);
    void setBaiduAppId(const QString &appId);
    void setApiKey(const QString &key);
    void setApiUrl(const QString &url);
    void setLibreTranslateUrl(const QString &url);

    // === Subtitle style setters ===
    void setDefaultFontSize(int size);
    void setDefaultFontColor(const QString &color);
    void setDefaultBorderColor(const QString &color);
    void setDefaultBorderWidth(int width);

    // === GPU & output setters ===
    void setUseGpuAccel(bool enable);
    void setQuality(int quality);

    Q_INVOKABLE void execute();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void requestStopAfterCount(int count, bool shutdown = false);

    // Settings delegation
    Q_INVOKABLE void testFfmpeg();
    Q_INVOKABLE void testWhisper();
    Q_INVOKABLE void testApiConnection();
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE bool isModelDownloaded(int modelIndex) const;
    Q_INVOKABLE void deleteModel(int modelIndex);
    Q_INVOKABLE QString modelFileName(int modelIndex) const;
    Q_INVOKABLE qint64 modelFileSize(int modelIndex) const;

signals:
    void inputPathChanged();
    void inputModeChanged();
    void recursiveChanged();
    void sourceLanguageChanged();
    void targetLanguageChanged();
    void translateMusicChanged();
    void subtitleStyleChanged();
    void outputModeChanged();
    void outputDirChanged();
    void keepWavChanged();
    void keepOriginalSrtChanged();
    void keepTranslatedSrtChanged();
    void enableAudioExtractionChanged();
    void enableTranscribeChanged();
    void enableTranslateChanged();
    void enableBurnSubtitleChanged();
    void statusMessageChanged();
    void progressChanged();
    void isProcessingChanged();
    void hasRecordsChanged();
    void recordsChanged();
    void currentStepChanged();
    void settingsRequired();  // Emitted when tools are not configured
    void logMessage(const QString &message);
    void logDetail(const QString &message);

    // === Tool paths signals ===
    void ffmpegPathChanged();
    void ffmpegStatusChanged();
    void ffmpegDetectingChanged();
    void whisperPathChanged();
    void whisperStatusChanged();
    void whisperDetectingChanged();
    void whisperModelChanged();
    void whisperModelDirChanged();
    void availableModelsChanged();
    void localModelPathChanged();
    void audioSegmentDurationChanged();

    // === Translate engine signals ===
    void translateEngineChanged();
    void baiduAppIdChanged();
    void apiKeyChanged();
    void apiUrlChanged();
    void apiTestResultChanged();
    void apiTestingChanged();
    void libreTranslateUrlChanged();
    void libreTranslateStatusChanged();

    // === Subtitle style signals ===
    void defaultFontSizeChanged();
    void defaultFontColorChanged();
    void defaultBorderColorChanged();
    void defaultBorderWidthChanged();

    // === GPU & output signals ===
    void useGpuAccelChanged();
    void qualityChanged();
    void gpuAccelInfoChanged();

private slots:
    void onAudioExtracted(bool success, const QString &audioPath, const QString &error);
    void onTranscribeFinished(bool success, const QString &srtPath, const QString &error);
    void onTranslateFinished(bool success, const QString &srtPath, const QString &error);
    void onBurnFinished(bool success, const QString &outputPath, const QString &error);
    void onWhisperProgress(double value);
    void onTranslateProgress(double value);
    void onFFmpegProgress(double value);

private:
    void processNextFile();
    void doScanWork();
    void processSingleFile(const QString &videoPath);
    void finalizeCurrentFile();
    void setStatusMessage(const QString &message);
    void setCurrentStep(int step, const QString &stepName);
    void setProgress(double value);
    void setIsProcessing(bool processing);
    void addRecord(const QString &originalPath, const QString &outputPath,
                   bool success, const QString &status);
    static QString stepNameForStep(int step);

    QThread m_workerThread;
    bool m_workerRunning = false;

    VideoSubtitleSettings *m_settings = nullptr;

    QString m_statusMessage;
    int m_currentStep = StepNone;
    QString m_currentStepName;
    double m_progress = 0.0;
    bool m_isProcessing = false;
    QList<QVariantMap> m_records;

    QStringList m_pendingFiles;
    int m_currentFileIndex = -1;
    int m_stopTargetIndex = -1;
    bool m_shutdownAfterStop = false;

    PluginLogger *m_logger = nullptr;
    WhisperService *m_whisperService;
    TranslateService *m_translateService;
    FFmpegService *m_ffmpegService;

    QString m_currentAudioPath;
    QString m_currentOriginalSrtPath;
    QString m_currentTranslatedSrtPath;
    QString m_currentVideoPath;
    QString m_currentOutputVideoPath;
};
