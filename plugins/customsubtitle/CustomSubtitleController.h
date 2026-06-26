#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class PluginLogger;
class CustomSubtitleSettings;
class SubtitleMatcher;
class FFmpegMergeService;
class VideoReplaceService;
#include "SubBrowserController.h"

class CustomSubtitleController : public QObject
{
    Q_OBJECT

    // === Sub Browser ===
    Q_PROPERTY(SubBrowserController* browserController READ browserController CONSTANT)

    // === State ===
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(double currentFileProgress READ currentFileProgress NOTIFY currentFileProgressChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(int currentStep READ currentStep NOTIFY currentStepChanged)
    Q_PROPERTY(int processedCount READ processedCount NOTIFY processedCountChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(bool stopRequested READ isStopRequested NOTIFY stopRequestedChanged)

    // === Config properties (delegated to CustomSubtitleSettings) ===
    Q_PROPERTY(QString subtitleDownloadPath READ subtitleDownloadPath WRITE setSubtitleDownloadPath NOTIFY subtitleDownloadPathChanged)
    Q_PROPERTY(QString videoSourcePath READ videoSourcePath WRITE setVideoSourcePath NOTIFY videoSourcePathChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(QString mergedOutputPath READ mergedOutputPath WRITE setMergedOutputPath NOTIFY mergedOutputPathChanged)
    Q_PROPERTY(QString ffmpegPath READ ffmpegPath WRITE setFfmpegPath NOTIFY ffmpegPathChanged)
    Q_PROPERTY(bool gpuAccel READ gpuAccel WRITE setGpuAccel NOTIFY gpuAccelChanged)
    Q_PROPERTY(bool removeSrtAfterReplace READ removeSrtAfterReplace WRITE setRemoveSrtAfterReplace NOTIFY removeSrtAfterReplaceChanged)
    Q_PROPERTY(bool backupOriginal READ backupOriginal WRITE setBackupOriginal NOTIFY backupOriginalChanged)
    Q_PROPERTY(QStringList enabledPreprocessors READ enabledPreprocessors WRITE setEnabledPreprocessors NOTIFY enabledPreprocessorsChanged)

    // === Step enum constants for QML ===
    Q_PROPERTY(int stepNone READ stepNone CONSTANT)
    Q_PROPERTY(int stepSearch READ stepSearch CONSTANT)
    Q_PROPERTY(int stepMatch READ stepMatch CONSTANT)
    Q_PROPERTY(int stepMerge READ stepMerge CONSTANT)
    Q_PROPERTY(int stepReplace READ stepReplace CONSTANT)

public:
    enum Step { StepNone = 0, StepSearch, StepMatch, StepMerge, StepReplace };
    explicit CustomSubtitleController(PluginLogger *logger, CustomSubtitleSettings *settings, QObject *parent = nullptr);
    ~CustomSubtitleController();

    // Getters
    QString subtitleDownloadPath() const;
    QString videoSourcePath() const;
    bool recursive() const;
    QString mergedOutputPath() const;
    bool gpuAccel() const;
    bool removeSrtAfterReplace() const;
    bool backupOriginal() const;
    QStringList enabledPreprocessors() const;
    QString statusMessage() const;
    double progress() const;
    double currentFileProgress() const;
    bool isProcessing() const;
    int currentStep() const;
    int processedCount() const;
    int totalCount() const;
    QString currentFile() const;
    bool isStopRequested() const { return m_gracefulStopRequested; }

    // Step enum read-only access for QML
    int stepNone() const { return StepNone; }
    int stepSearch() const { return StepSearch; }
    int stepMatch() const { return StepMatch; }
    int stepMerge() const { return StepMerge; }
    int stepReplace() const { return StepReplace; }

    // Setters
    QString ffmpegPath() const;
    void setSubtitleDownloadPath(const QString &path);
    void setVideoSourcePath(const QString &path);
    void setRecursive(bool recursive);
    void setMergedOutputPath(const QString &path);
    void setFfmpegPath(const QString &path);
    void setGpuAccel(bool enable);
    void setRemoveSrtAfterReplace(bool remove);
    void setBackupOriginal(bool backup);
    void setEnabledPreprocessors(const QStringList &ops);

    // === Sub Browser ===
    SubBrowserController* browserController() const;

    // === Actions (called from QML) ===
    Q_INVOKABLE void matchAndMoveSubtitles();       // Step 2
    Q_INVOKABLE void mergeSubtitleToVideo();         // Step 3
    Q_INVOKABLE void replaceOriginalVideo();         // Step 4
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void requestStopAfterCount(int count, bool shutdown = false);  // 完成 count 个后退出
    Q_INVOKABLE void reset();

signals:
    void subtitleDownloadPathChanged();
    void videoSourcePathChanged();
    void recursiveChanged();
    void mergedOutputPathChanged();
    void ffmpegPathChanged();
    void gpuAccelChanged();
    void removeSrtAfterReplaceChanged();
    void backupOriginalChanged();
    void enabledPreprocessorsChanged();
    void statusMessageChanged();
    void progressChanged();
    void currentFileProgressChanged();
    void isProcessingChanged();
    void currentStepChanged();
    void processedCountChanged();
    void totalCountChanged();
    void currentFileChanged();
    void logMessage(const QString &message);
    void stopRequestedChanged();

private slots:
    void onMatchFinished(bool success, const QString &error);
    void onMergeFinished(bool success, const QString &error);
    void onReplaceFinished(bool success, const QString &error);

private:
    void setStatusMessage(const QString &msg);
    void setCurrentStep(int step);
    void setProgress(double value);
    void setCurrentFileProgress(double value);
    void setIsProcessing(bool processing);
    void setProcessedCount(int count);
    void setTotalCount(int count);
    void setCurrentFile(const QString &path);

    QString m_statusMessage;
    double m_progress = 0.0;
    double m_currentFileProgress = 0.0;
    bool m_isProcessing = false;
    int m_currentStep = StepNone;

    CustomSubtitleSettings *m_settings = nullptr;
    PluginLogger *m_logger;
    SubtitleMatcher *m_matcher;
    FFmpegMergeService *m_mergeService;
    VideoReplaceService *m_replaceService;
    SubBrowserController *m_browserController;
    int m_processedCount = 0;
    int m_totalCount = 0;
    QString m_currentFile;
    bool m_gracefulStopRequested = false;
    bool m_shutdownAfterStop = false;
    bool m_removeSrtAfterReplace = true;
};
