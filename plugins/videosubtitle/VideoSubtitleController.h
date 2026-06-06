#pragma once

#include <QObject>
#include <QVariantList>
#include <QDir>

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
    Q_PROPERTY(bool bilingual READ bilingual WRITE setBilingual NOTIFY bilingualChanged)

    // === Output settings ===
    Q_PROPERTY(int outputMode READ outputMode WRITE setOutputMode NOTIFY outputModeChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool keepOriginalSrt READ keepOriginalSrt WRITE setKeepOriginalSrt NOTIFY keepOriginalSrtChanged)

    // === Status ===
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)
    Q_PROPERTY(QString currentStep READ currentStep NOTIFY currentStepChanged)

public:
    explicit VideoSubtitleController(QObject *parent = nullptr);

    // Getters
    QString inputPath() const;
    int inputMode() const;
    bool recursive() const;
    QString sourceLanguage() const;
    QString targetLanguage() const;
    int subtitleStyle() const;
    bool bilingual() const;
    int outputMode() const;
    QString outputDir() const;
    bool keepOriginalSrt() const;
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
    void setBilingual(bool bilingual);
    void setOutputMode(int mode);
    void setOutputDir(const QString &dir);
    void setKeepOriginalSrt(bool keep);

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
    void bilingualChanged();
    void outputModeChanged();
    void outputDirChanged();
    void keepOriginalSrtChanged();
    void statusMessageChanged();
    void progressChanged();
    void isProcessingChanged();
    void hasRecordsChanged();
    void recordsChanged();
    void currentStepChanged();
    void settingsRequired();  // Emitted when tools are not configured

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
    void setStatusMessage(const QString &message);
    void setCurrentStep(const QString &step);
    void setProgress(double value);
    void setIsProcessing(bool processing);
    void addRecord(const QString &originalPath, const QString &outputPath,
                   bool success, const QString &status);
    bool isVideoFile(const QString &fileName) const;

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

    QString m_inputPath;
    int m_inputMode = 0;
    bool m_recursive = false;
    QString m_sourceLanguage = "auto";
    QString m_targetLanguage = "zh";
    int m_subtitleStyle = 0;
    bool m_bilingual = true;
    int m_outputMode = 0;
    QString m_outputDir;
    bool m_keepOriginalSrt = true;
    QString m_statusMessage;
    QString m_currentStep;
    double m_progress = 0.0;
    bool m_isProcessing = false;
    QList<QVariantMap> m_records;

    QStringList m_pendingFiles;
    int m_currentFileIndex = -1;

    WhisperService *m_whisperService;
    TranslateService *m_translateService;
    FFmpegService *m_ffmpegService;

    QString m_currentAudioPath;
    QString m_currentOriginalSrtPath;
    QString m_currentTranslatedSrtPath;
    QString m_currentVideoPath;
    QString m_currentOutputVideoPath;

    const QStringList m_videoExtensions = {
        ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".ts"
    };
};
