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

    Q_INVOKABLE void execute();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void requestStopAfterCount(int count);

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
    // Read settings through VideoSubtitleSettings
    QString ffmpegPath() const;
    QString whisperPath() const;
    QString whisperModelPath() const;
    QString apiKey() const;
    QString apiUrl() const;
    QString baiduAppId() const;
    int translateEngine() const;
    int defaultFontSize() const;
    QString defaultFontColor() const;
    QString defaultBorderColor() const;
    int defaultBorderWidth() const;
    int audioSegmentDuration() const;

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
