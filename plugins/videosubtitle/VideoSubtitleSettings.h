#pragma once

#include <QObject>
#include <QSettings>
#include <QVariantList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "PluginLogger.h"

class VideoSubtitleSettings : public QObject
{
    Q_OBJECT

    // 共享 config.ini 路径
    static QString configPath() { return pluginConfigFilePath(); }

    // === Tool paths ===
    Q_PROPERTY(QString ffmpegPath READ ffmpegPath WRITE setFfmpegPath NOTIFY ffmpegPathChanged)
    Q_PROPERTY(QString ffmpegStatus READ ffmpegStatus NOTIFY ffmpegStatusChanged)
    Q_PROPERTY(QString whisperPath READ whisperPath WRITE setWhisperPath NOTIFY whisperPathChanged)
    Q_PROPERTY(QString whisperStatus READ whisperStatus NOTIFY whisperStatusChanged)

    // === Whisper model management ===
    Q_PROPERTY(int whisperModel READ whisperModel WRITE setWhisperModel NOTIFY whisperModelChanged)
    Q_PROPERTY(QString whisperModelDir READ whisperModelDir WRITE setWhisperModelDir NOTIFY whisperModelDirChanged)
    Q_PROPERTY(QVariantList availableModels READ availableModels NOTIFY availableModelsChanged)
    Q_PROPERTY(QString localModelPath READ localModelPath WRITE setLocalModelPath NOTIFY localModelPathChanged)

    // === Translation API ===
    Q_PROPERTY(int translateEngine READ translateEngine WRITE setTranslateEngine NOTIFY translateEngineChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString baiduAppId READ baiduAppId WRITE setBaiduAppId NOTIFY baiduAppIdChanged)
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)
    Q_PROPERTY(QString apiTestResult READ apiTestResult NOTIFY apiTestResultChanged)
    Q_PROPERTY(bool apiTesting READ apiTesting NOTIFY apiTestingChanged)
    Q_PROPERTY(QStringList translateEngineNames READ translateEngineNames CONSTANT)

    // === Segment duration for transcription ===
    Q_PROPERTY(int audioSegmentDuration READ audioSegmentDuration WRITE setAudioSegmentDuration NOTIFY audioSegmentDurationChanged)

    // === Subtitle style ===
    Q_PROPERTY(int subtitleStyle READ subtitleStyle WRITE setSubtitleStyle NOTIFY subtitleStyleChanged)

    // === Output file retention ===
    Q_PROPERTY(bool keepWav READ keepWav WRITE setKeepWav NOTIFY keepWavChanged)
    Q_PROPERTY(bool keepOriginalSrt READ keepOriginalSrt WRITE setKeepOriginalSrt NOTIFY keepOriginalSrtChanged)
    Q_PROPERTY(bool keepTranslatedSrt READ keepTranslatedSrt WRITE setKeepTranslatedSrt NOTIFY keepTranslatedSrtChanged)

    // === Font config ===
    Q_PROPERTY(int defaultFontSize READ defaultFontSize WRITE setDefaultFontSize NOTIFY defaultFontSizeChanged)
    Q_PROPERTY(QString defaultFontColor READ defaultFontColor WRITE setDefaultFontColor NOTIFY defaultFontColorChanged)
    Q_PROPERTY(QString defaultBorderColor READ defaultBorderColor WRITE setDefaultBorderColor NOTIFY defaultBorderColorChanged)
    Q_PROPERTY(int defaultBorderWidth READ defaultBorderWidth WRITE setDefaultBorderWidth NOTIFY defaultBorderWidthChanged)

public:
    explicit VideoSubtitleSettings(QObject *parent = nullptr);

    // Getters
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
    int subtitleStyle() const;
    bool keepWav() const;
    bool keepOriginalSrt() const;
    bool keepTranslatedSrt() const;
    int defaultFontSize() const;
    QString defaultFontColor() const;
    QString defaultBorderColor() const;
    int defaultBorderWidth() const;

    // Setters
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

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE void testFfmpeg();
    Q_INVOKABLE void testWhisper();
    Q_INVOKABLE void testApiConnection();
    Q_INVOKABLE QStringList translateEngineNames() const;
    Q_INVOKABLE bool isModelDownloaded(int modelIndex) const;
    Q_INVOKABLE void deleteModel(int modelIndex);
    Q_INVOKABLE QString modelFileName(int modelIndex) const;
    Q_INVOKABLE qint64 modelFileSize(int modelIndex) const;

signals:
    void ffmpegPathChanged();
    void ffmpegStatusChanged();
    void whisperPathChanged();
    void whisperStatusChanged();
    void whisperModelChanged();
    void whisperModelDirChanged();
    void availableModelsChanged();
    void localModelPathChanged();
    void audioSegmentDurationChanged();
    void translateEngineChanged();
    void apiKeyChanged();
    void baiduAppIdChanged();
    void apiUrlChanged();
    void apiTestResultChanged();
    void apiTestingChanged();
    void subtitleStyleChanged();
    void keepWavChanged();
    void keepOriginalSrtChanged();
    void keepTranslatedSrtChanged();
    void defaultFontSizeChanged();
    void defaultFontColorChanged();
    void defaultBorderColorChanged();
    void defaultBorderWidthChanged();
    void settingsChanged();

private:
    void detectTools();
    void updateApiUrlForEngine(int engine);
    QString defaultApiUrl(int engine) const;
    // [extension]: 新增翻译引擎时在此添加对应的 testXxxConnection() 声明
    void testBaiduConnection();

    QSettings m_settings;
    QString m_ffmpegPath;
    QString m_ffmpegStatus;
    QString m_whisperPath;
    QString m_whisperStatus;
    int m_whisperModel = 3; // medium (default)
    QString m_whisperModelDir;
    QString m_localModelPath;
    int m_audioSegmentDuration = 10; // seconds, 0 = disabled
    int m_translateEngine = 0; // 百度翻译
    QString m_apiKey;
    QString m_baiduAppId;
    QString m_apiUrl;
    QString m_apiTestResult;
    bool m_apiTesting = false;
    int m_subtitleStyle = 0;
    bool m_keepWav = true;
    bool m_keepOriginalSrt = true;
    bool m_keepTranslatedSrt = true;
    int m_defaultFontSize = 20;
    QString m_defaultFontColor = "#FFFFFF";
    QString m_defaultBorderColor = "#000000";
    int m_defaultBorderWidth = 2;

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_testReply;

    struct ModelInfo {
        QString name;
        QString fileName;
        qint64 fileSize;
    };
    static const QVector<ModelInfo> MODEL_INFOS;
};
