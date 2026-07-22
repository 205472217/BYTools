#pragma once

#include <QObject>
#include <QVariantList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFutureWatcher>

class PluginLogger;

struct ToolsDetectResult {
    QString ffmpegStatus;
    QString gpuAccelInfo;
};

class VideoSubtitleSettings : public QObject
{
    Q_OBJECT

public:
    explicit VideoSubtitleSettings(PluginLogger *logger, QObject *parent = nullptr);

    // ── 设置页 getter ──
    QString ffmpegPath() const;
    QString ffmpegStatus() const;
    QString whisperPath() const;
    QString whisperStatus() const;
    int whisperModel() const;
    QString whisperModelDir() const;
    QVariantList availableModels() const;
    QString localModelPath() const;
    int audioSegmentDuration() const;
    int translateEngine() const;
    QString apiKey() const;
    QString baiduAppId() const;
    QString apiUrl() const;
    QString apiTestResult() const;
    bool apiTesting() const;
    QStringList translateEngineNames() const;
    QString libreTranslateUrl() const;
    QString libreTranslateStatus() const;
    int subtitleStyle() const;
    bool keepWav() const;
    bool keepOriginalSrt() const;
    bool keepTranslatedSrt() const;
    int defaultFontSize() const;
    QString defaultFontColor() const;
    QString defaultBorderColor() const;
    int defaultBorderWidth() const;
    bool useGpuAccel() const;
    int quality() const;
    QString gpuAccelInfo() const;
    bool ffmpegDetecting() const { return m_ffmpegDetecting; }
    bool whisperDetecting() const { return m_whisperDetecting; }

    // ── 主页面 getter ──
    QString inputPath() const;
    int inputMode() const;
    bool recursive() const;
    QString sourceLanguage() const;
    QString targetLanguage() const;
    bool translateMusic() const;
    int outputMode() const;
    QString outputDir() const;
    bool enableAudioExtraction() const;
    bool enableTranscribe() const;
    bool enableTranslate() const;
    bool enableBurnSubtitle() const;

    // ── 设置页 setter ──
    void setFfmpegPath(const QString &path);
    void setWhisperPath(const QString &path);
    void setWhisperModel(int model);
    void setWhisperModelDir(const QString &path);
    void setLocalModelPath(const QString &path);
    void setAudioSegmentDuration(int seconds);
    void setTranslateEngine(int engine);
    void setApiKey(const QString &key);
    void setBaiduAppId(const QString &appId);
    void setApiUrl(const QString &url);
    void setSubtitleStyle(int style);
    void setKeepWav(bool keep);
    void setKeepOriginalSrt(bool keep);
    void setKeepTranslatedSrt(bool keep);
    void setDefaultFontSize(int size);
    void setDefaultFontColor(const QString &color);
    void setDefaultBorderColor(const QString &color);
    void setDefaultBorderWidth(int width);
    void setUseGpuAccel(bool enable);
    void setQuality(int quality);
    void setLibreTranslateUrl(const QString &url);

    // ── 主页面 setter ──
    void setInputPath(const QString &path);
    void setInputMode(int mode);
    void setRecursive(bool recursive);
    void setSourceLanguage(const QString &lang);
    void setTargetLanguage(const QString &lang);
    void setTranslateMusic(bool enabled);
    void setOutputMode(int mode);
    void setOutputDir(const QString &dir);
    void setEnableAudioExtraction(bool enabled);
    void setEnableTranscribe(bool enabled);
    void setEnableTranslate(bool enabled);
    void setEnableBurnSubtitle(bool enabled);

    // ── 其他功能函数 ──
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE void testFfmpeg();
    Q_INVOKABLE void testWhisper();
    Q_INVOKABLE void testApiConnection();
    Q_INVOKABLE bool isModelDownloaded(int modelIndex) const;
    Q_INVOKABLE void deleteModel(int modelIndex);
    Q_INVOKABLE QString modelFileName(int modelIndex) const;
    Q_INVOKABLE qint64 modelFileSize(int modelIndex) const;
    QString whisperModelPath() const;

signals:
    void detectionFinished();

private:
    void startAsyncDetection();
    void onToolsDetectionFinished();
    static ToolsDetectResult runToolsDetection(const QString &ffmpegPath, PluginLogger *logger);
    QString findBundled(const QString &fileName) const;
    void updateApiUrlForEngine(int engine);
    QString defaultApiUrl(int engine) const;
    void testBaiduConnection();
    void testLibreTranslateConnection();

    PluginLogger *m_logger = nullptr;
    QFutureWatcher<ToolsDetectResult> *m_detectWatcher = nullptr;
    bool m_ffmpegDetecting = false;
    bool m_whisperDetecting = false;

    // ── 设置页成员 ──
    QString m_ffmpegPath;
    QString m_ffmpegStatus;
    QString m_whisperPath;
    QString m_whisperStatus;
    int m_whisperModel = 3;
    QString m_whisperModelDir;
    QString m_localModelPath;
    int m_audioSegmentDuration = 10;
    int m_translateEngine = 0;
    QString m_apiKey;
    QString m_baiduAppId;
    QString m_apiUrl;
    QString m_apiTestResult;
    bool m_apiTesting = false;
    int m_subtitleStyle = 0;
    bool m_keepWav = true;
    bool m_keepOriginalSrt = true;
    bool m_keepTranslatedSrt = true;
    int m_defaultFontSize = 18;
    QString m_defaultFontColor = "#FFFFFF";
    QString m_defaultBorderColor = "#000000";
    int m_defaultBorderWidth = 2;
    bool m_useGpuAccel = false;
    int m_quality = 0;
    QString m_gpuAccelInfo;
    QString m_libreTranslateUrl;
    QString m_libreTranslateStatus;
    // ── 主页面成员 ──
    QString m_inputPath;
    int m_inputMode = 0;
    bool m_recursive = false;
    QString m_sourceLanguage = "auto";
    QString m_targetLanguage = "zh";
    bool m_translateMusic = false;
    int m_outputMode = 0;
    QString m_outputDir;
    bool m_enableAudioExtraction = true;
    bool m_enableTranscribe = true;
    bool m_enableTranslate = true;
    bool m_enableBurnSubtitle = true;

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_testReply;

    struct ModelInfo {
        QString name;
        QString fileName;
        qint64 fileSize;
    };
    static const QVector<ModelInfo> MODEL_INFOS;
};
