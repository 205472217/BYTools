#pragma once

#include <QObject>
#include <QVariantList>
#include <QDir>

class PluginLogger;
class WhisperService;
class TranslateService;
class FFmpegService;

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
    Q_PROPERTY(QString currentStep READ currentStep NOTIFY currentStepChanged)

public:
    explicit VideoSubtitleController(PluginLogger *logger, QObject *parent = nullptr);

    // Getters
    QString inputPath() const;
    int inputMode() const;
    bool recursive() const;
    QString sourceLanguage() const;
    QString targetLanguage() const;
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
    QString currentStep() const;

    // Setters
    void setInputPath(const QString &path);
    void setInputMode(int mode);
    void setRecursive(bool recursive);
    void setSourceLanguage(const QString &lang);
    void setTargetLanguage(const QString &lang);
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

signals:
    void inputPathChanged();
    void inputModeChanged();
    void recursiveChanged();
    void sourceLanguageChanged();
    void targetLanguageChanged();
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
    void processSingleFile(const QString &videoPath);
    void finalizeCurrentFile();
    void setStatusMessage(const QString &message);
    void setCurrentStep(const QString &step);
    void setProgress(double value);
    void setIsProcessing(bool processing);
    void addRecord(const QString &originalPath, const QString &outputPath,
                   bool success, const QString &status);
    // Read settings from QSettings
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

    QString m_inputPath;
    int m_inputMode = 0;
    bool m_recursive = false;
    QString m_sourceLanguage = "auto";
    QString m_targetLanguage = "zh";
    int m_outputMode = 0;
    QString m_outputDir;
    QString m_statusMessage;
    QString m_currentStep;
    double m_progress = 0.0;
    bool m_isProcessing = false;
    QList<QVariantMap> m_records;

    // Step control flags
    bool m_enableAudioExtraction = true;
    bool m_enableTranscribe = true;
    bool m_enableTranslate = true;
    bool m_enableBurnSubtitle = true;

    QStringList m_pendingFiles;
    int m_currentFileIndex = -1;

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
