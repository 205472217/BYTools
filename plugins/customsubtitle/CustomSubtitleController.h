#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class SubtitleMatcher;
class FFmpegMergeService;
class VideoReplaceService;

class CustomSubtitleController : public QObject
{
    Q_OBJECT

    // === Path config ===
    Q_PROPERTY(QString subtitleDownloadPath READ subtitleDownloadPath WRITE setSubtitleDownloadPath NOTIFY subtitleDownloadPathChanged)
    Q_PROPERTY(QString videoSourcePath READ videoSourcePath WRITE setVideoSourcePath NOTIFY videoSourcePathChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(QString mergedOutputPath READ mergedOutputPath WRITE setMergedOutputPath NOTIFY mergedOutputPathChanged)

    // === Options ===
    Q_PROPERTY(bool gpuAccel READ gpuAccel WRITE setGpuAccel NOTIFY gpuAccelChanged)
    Q_PROPERTY(bool removeSrtAfterReplace READ removeSrtAfterReplace WRITE setRemoveSrtAfterReplace NOTIFY removeSrtAfterReplaceChanged)
    Q_PROPERTY(bool backupOriginal READ backupOriginal WRITE setBackupOriginal NOTIFY backupOriginalChanged)

    // === State ===
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(QString currentStep READ currentStep NOTIFY currentStepChanged)

public:
    explicit CustomSubtitleController(QObject *parent = nullptr);
    ~CustomSubtitleController();

    // Getters
    QString subtitleDownloadPath() const;
    QString videoSourcePath() const;
    bool recursive() const;
    QString mergedOutputPath() const;
    bool gpuAccel() const;
    bool removeSrtAfterReplace() const;
    bool backupOriginal() const;
    QString statusMessage() const;
    double progress() const;
    bool isProcessing() const;
    QString currentStep() const;

    // Setters
    void setSubtitleDownloadPath(const QString &path);
    void setVideoSourcePath(const QString &path);
    void setRecursive(bool recursive);
    void setMergedOutputPath(const QString &path);
    void setGpuAccel(bool enable);
    void setRemoveSrtAfterReplace(bool remove);
    void setBackupOriginal(bool backup);

    // === Actions (called from QML) ===
    Q_INVOKABLE void matchAndMoveSubtitles();       // Step 2
    Q_INVOKABLE void mergeSubtitleToVideo();         // Step 3
    Q_INVOKABLE void replaceOriginalVideo();         // Step 4
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void reset();

    // === URL helpers ===
    /// Return the ffmpeg path from shared config (same config.ini as VideoSubtitle)
    Q_INVOKABLE QString ffmpegPath() const;

signals:
    void subtitleDownloadPathChanged();
    void videoSourcePathChanged();
    void recursiveChanged();
    void mergedOutputPathChanged();
    void gpuAccelChanged();
    void removeSrtAfterReplaceChanged();
    void backupOriginalChanged();
    void statusMessageChanged();
    void progressChanged();
    void isProcessingChanged();
    void currentStepChanged();
    void logMessage(const QString &message);

private slots:
    void onMatchFinished(bool success, const QString &error);
    void onMergeFinished(bool success, const QString &error);
    void onReplaceFinished(bool success, const QString &error);

private:
    void setStatusMessage(const QString &msg);
    void setCurrentStep(const QString &step);
    void setProgress(double value);
    void setIsProcessing(bool processing);

    QString m_subtitleDownloadPath;
    QString m_videoSourcePath;
    bool m_recursive = false;
    QString m_mergedOutputPath;
    bool m_gpuAccel = false;
    bool m_removeSrtAfterReplace = true;
    bool m_backupOriginal = false;
    QString m_statusMessage;
    double m_progress = 0.0;
    bool m_isProcessing = false;
    QString m_currentStep;

    SubtitleMatcher *m_matcher;
    FFmpegMergeService *m_mergeService;
    VideoReplaceService *m_replaceService;
};
