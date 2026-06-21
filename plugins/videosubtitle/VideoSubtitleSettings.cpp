#include "VideoSubtitleSettings.h"
#include "VideoSubtitlePlugin.h"
#include "Logger.h"
#include "FFmpegService.h"
#include "FfmpegUtils.h"
#include "WhisperService.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QRandomGenerator>

const QVector<VideoSubtitleSettings::ModelInfo> VideoSubtitleSettings::MODEL_INFOS = {
    {"tiny",   "ggml-tiny.bin",   77701664LL},
    {"base",   "ggml-base.bin",   147507174LL},
    {"small",  "ggml-small.bin",  488018367LL},
    {"medium", "ggml-medium.bin", 1533860498LL},
    {"large",  "ggml-large-v3.bin", 3095032444LL}
};

VideoSubtitleSettings::VideoSubtitleSettings(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(configPath(), QSettings::IniFormat)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_testReply(nullptr)
    , m_libreTranslateUrl("http://localhost:5000")
{
    m_settings.beginGroup(VideoSubtitlePlugin::kIniSection);
    m_whisperModelDir = QCoreApplication::applicationDirPath() + "/plugins/videosubtitle";
    loadSettings();
    m_logger->info("插件设置已加载");
}

// ── 设置页 getter ──
QString VideoSubtitleSettings::ffmpegPath() const { return m_ffmpegPath; }
QString VideoSubtitleSettings::ffmpegStatus() const { return m_ffmpegStatus; }
QString VideoSubtitleSettings::whisperPath() const { return m_whisperPath; }
QString VideoSubtitleSettings::whisperStatus() const { return m_whisperStatus; }
int VideoSubtitleSettings::whisperModel() const { return m_whisperModel; }
QString VideoSubtitleSettings::whisperModelDir() const { return m_whisperModelDir; }
QVariantList VideoSubtitleSettings::availableModels() const
{
    QVariantList list;
    for (int i = 0; i < MODEL_INFOS.size(); ++i) {
        QVariantMap map;
        map["name"] = MODEL_INFOS[i].name;
        map["fileName"] = MODEL_INFOS[i].fileName;
        map["fileSize"] = MODEL_INFOS[i].fileSize;
        map["downloaded"] = isModelDownloaded(i);
        list.append(map);
    }
    return list;
}
QString VideoSubtitleSettings::localModelPath() const { return m_localModelPath; }
int VideoSubtitleSettings::audioSegmentDuration() const { return m_audioSegmentDuration; }
int VideoSubtitleSettings::translateEngine() const { return m_translateEngine; }
QString VideoSubtitleSettings::apiKey() const { return m_apiKey; }
QString VideoSubtitleSettings::baiduAppId() const { return m_baiduAppId; }
QString VideoSubtitleSettings::apiUrl() const { return m_apiUrl; }
QString VideoSubtitleSettings::apiTestResult() const { return m_apiTestResult; }
bool VideoSubtitleSettings::apiTesting() const { return m_apiTesting; }
QStringList VideoSubtitleSettings::translateEngineNames() const
{
    return {"百度翻译", "LibreTranslate"};
}
QString VideoSubtitleSettings::libreTranslateUrl() const { return m_libreTranslateUrl; }
QString VideoSubtitleSettings::libreTranslateStatus() const { return m_libreTranslateStatus; }
int VideoSubtitleSettings::subtitleStyle() const { return m_subtitleStyle; }
bool VideoSubtitleSettings::keepWav() const { return m_keepWav; }
bool VideoSubtitleSettings::keepOriginalSrt() const { return m_keepOriginalSrt; }
bool VideoSubtitleSettings::keepTranslatedSrt() const { return m_keepTranslatedSrt; }
int VideoSubtitleSettings::defaultFontSize() const { return m_defaultFontSize; }
QString VideoSubtitleSettings::defaultFontColor() const { return m_defaultFontColor; }
QString VideoSubtitleSettings::defaultBorderColor() const { return m_defaultBorderColor; }
int VideoSubtitleSettings::defaultBorderWidth() const { return m_defaultBorderWidth; }
bool VideoSubtitleSettings::useGpuAccel() const { return m_useGpuAccel; }
QString VideoSubtitleSettings::gpuAccelInfo() const { return m_gpuAccelInfo; }

// ── 主页面 getter ──
QString VideoSubtitleSettings::inputPath() const { return m_inputPath; }
int VideoSubtitleSettings::inputMode() const { return m_inputMode; }
bool VideoSubtitleSettings::recursive() const { return m_recursive; }
QString VideoSubtitleSettings::sourceLanguage() const { return m_sourceLanguage; }
QString VideoSubtitleSettings::targetLanguage() const { return m_targetLanguage; }
bool VideoSubtitleSettings::translateMusic() const { return m_translateMusic; }
int VideoSubtitleSettings::outputMode() const { return m_outputMode; }
QString VideoSubtitleSettings::outputDir() const { return m_outputDir; }
bool VideoSubtitleSettings::enableAudioExtraction() const { return m_enableAudioExtraction; }
bool VideoSubtitleSettings::enableTranscribe() const { return m_enableTranscribe; }
bool VideoSubtitleSettings::enableTranslate() const { return m_enableTranslate; }
bool VideoSubtitleSettings::enableBurnSubtitle() const { return m_enableBurnSubtitle; }

// ── 设置页 setter ──
void VideoSubtitleSettings::setFfmpegPath(const QString &path)
{
    if (m_ffmpegPath != path) {
        m_ffmpegPath = path;
        emit ffmpegPathChanged();
        if (isFFmpegAvailable(path, m_logger)) {
            QString ver = ffmpegVersion(path);
            m_ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
        } else {
            m_ffmpegStatus = path.isEmpty() ? "未配置" : "无法找到 FFmpeg";
        }
        emit ffmpegStatusChanged();
        if (isFFmpegAvailable(path, m_logger)) {
            m_gpuAccelInfo = "GPU 加速: " + FFmpegService::hardwareAccelName(path);
        } else {
            m_gpuAccelInfo = "GPU 加速: 需先配置 FFmpeg";
        }
        emit gpuAccelInfoChanged();
    }
}
void VideoSubtitleSettings::setWhisperPath(const QString &path)
{
    if (m_whisperPath != path) {
        m_whisperPath = path;
        emit whisperPathChanged();
        if (!path.isEmpty() && QFileInfo::exists(path)) {
            m_whisperStatus = "已检测到 whisper.cpp";
        } else {
            m_whisperStatus = path.isEmpty() ? "未配置" : "无法找到 whisper.cpp";
        }
        emit whisperStatusChanged();
    }
}
void VideoSubtitleSettings::setWhisperModel(int model)
{
    if (m_whisperModel != model && model >= 0 && model < MODEL_INFOS.size()) {
        m_whisperModel = model;
        emit whisperModelChanged();
    }
}
void VideoSubtitleSettings::setWhisperModelDir(const QString &path)
{
    if (m_whisperModelDir != path) {
        m_whisperModelDir = path;
        emit whisperModelDirChanged();
        emit availableModelsChanged();
    }
}
void VideoSubtitleSettings::setLocalModelPath(const QString &path)
{
    if (m_localModelPath != path) {
        m_localModelPath = path;
        emit localModelPathChanged();
    }
}
void VideoSubtitleSettings::setAudioSegmentDuration(int seconds)
{
    if (m_audioSegmentDuration != seconds) {
        m_audioSegmentDuration = seconds;
        emit audioSegmentDurationChanged();
    }
}
void VideoSubtitleSettings::setTranslateEngine(int engine)
{
    if (m_translateEngine != engine) {
        m_translateEngine = engine;
        updateApiUrlForEngine(engine);
        emit translateEngineChanged();
        emit apiUrlChanged();
    }
}
void VideoSubtitleSettings::setApiKey(const QString &key)
{
    if (m_apiKey != key) {
        m_apiKey = key;
        emit apiKeyChanged();
    }
}
void VideoSubtitleSettings::setBaiduAppId(const QString &appId)
{
    if (m_baiduAppId != appId) {
        m_baiduAppId = appId;
        emit baiduAppIdChanged();
    }
}
void VideoSubtitleSettings::setApiUrl(const QString &url)
{
    if (m_apiUrl != url) {
        m_apiUrl = url;
        emit apiUrlChanged();
    }
}
void VideoSubtitleSettings::setSubtitleStyle(int style)
{
    if (m_subtitleStyle != style) {
        m_subtitleStyle = style;
        emit subtitleStyleChanged();
    }
}
void VideoSubtitleSettings::setKeepWav(bool keep)
{
    if (m_keepWav != keep) {
        m_keepWav = keep;
        emit keepWavChanged();
    }
}
void VideoSubtitleSettings::setKeepOriginalSrt(bool keep)
{
    if (m_keepOriginalSrt != keep) {
        m_keepOriginalSrt = keep;
        emit keepOriginalSrtChanged();
    }
}
void VideoSubtitleSettings::setKeepTranslatedSrt(bool keep)
{
    if (m_keepTranslatedSrt != keep) {
        m_keepTranslatedSrt = keep;
        emit keepTranslatedSrtChanged();
    }
}
void VideoSubtitleSettings::setDefaultFontSize(int size)
{
    if (m_defaultFontSize != size) {
        m_defaultFontSize = size;
        emit defaultFontSizeChanged();
    }
}
void VideoSubtitleSettings::setDefaultFontColor(const QString &color)
{
    if (m_defaultFontColor != color) {
        m_defaultFontColor = color;
        emit defaultFontColorChanged();
    }
}
void VideoSubtitleSettings::setDefaultBorderColor(const QString &color)
{
    if (m_defaultBorderColor != color) {
        m_defaultBorderColor = color;
        emit defaultBorderColorChanged();
    }
}
void VideoSubtitleSettings::setDefaultBorderWidth(int width)
{
    if (m_defaultBorderWidth != width) {
        m_defaultBorderWidth = width;
        emit defaultBorderWidthChanged();
    }
}
void VideoSubtitleSettings::setUseGpuAccel(bool enable)
{
    if (m_useGpuAccel != enable) {
        m_useGpuAccel = enable;
        emit useGpuAccelChanged();
    }
}
void VideoSubtitleSettings::setLibreTranslateUrl(const QString &url)
{
    if (m_libreTranslateUrl != url) {
        m_libreTranslateUrl = url;
        emit libreTranslateUrlChanged();
    }
}

// ── 主页面 setter ──
void VideoSubtitleSettings::setInputPath(const QString &path)
{
    if (m_inputPath != path) {
        m_inputPath = path;
        emit inputPathChanged();
        m_settings.setValue("inputPath", path);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setInputMode(int mode)
{
    if (m_inputMode != mode) {
        m_inputMode = mode;
        emit inputModeChanged();
        m_settings.setValue("inputMode", mode);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        emit recursiveChanged();
        m_settings.setValue("recursive", recursive);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setSourceLanguage(const QString &lang)
{
    if (m_sourceLanguage != lang) {
        m_sourceLanguage = lang;
        emit sourceLanguageChanged();
        m_settings.setValue("sourceLanguage", lang);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setTargetLanguage(const QString &lang)
{
    if (m_targetLanguage != lang) {
        m_targetLanguage = lang;
        emit targetLanguageChanged();
        m_settings.setValue("targetLanguage", lang);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setTranslateMusic(bool enabled)
{
    if (m_translateMusic != enabled) {
        m_translateMusic = enabled;
        emit translateMusicChanged();
        m_settings.setValue("translateMusic", enabled);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setOutputMode(int mode)
{
    if (m_outputMode != mode) {
        m_outputMode = mode;
        emit outputModeChanged();
        m_settings.setValue("outputMode", mode);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setOutputDir(const QString &dir)
{
    if (m_outputDir != dir) {
        m_outputDir = dir;
        emit outputDirChanged();
        m_settings.setValue("outputDir", dir);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setEnableAudioExtraction(bool enabled)
{
    if (m_enableAudioExtraction != enabled) {
        m_enableAudioExtraction = enabled;
        emit enableAudioExtractionChanged();
        m_settings.setValue("enableAudioExtraction", enabled);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setEnableTranscribe(bool enabled)
{
    if (m_enableTranscribe != enabled) {
        m_enableTranscribe = enabled;
        emit enableTranscribeChanged();
        m_settings.setValue("enableTranscribe", enabled);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setEnableTranslate(bool enabled)
{
    if (m_enableTranslate != enabled) {
        m_enableTranslate = enabled;
        emit enableTranslateChanged();
        m_settings.setValue("enableTranslate", enabled);
        m_settings.sync();
    }
}
void VideoSubtitleSettings::setEnableBurnSubtitle(bool enabled)
{
    if (m_enableBurnSubtitle != enabled) {
        m_enableBurnSubtitle = enabled;
        emit enableBurnSubtitleChanged();
        m_settings.setValue("enableBurnSubtitle", enabled);
        m_settings.sync();
    }
}

// ── 其他功能函数 ──
void VideoSubtitleSettings::loadSettings()
{
    m_settings.sync();

    m_ffmpegPath = m_settings.value("ffmpegPath").toString();
    m_whisperPath = m_settings.value("whisperPath").toString();
    m_whisperModel = m_settings.value("whisperModel", 3).toInt();
    m_whisperModelDir = m_settings.value("whisperModelDir", m_whisperModelDir).toString();
    m_localModelPath = m_settings.value("localModelPath").toString();
    m_audioSegmentDuration = m_settings.value("audioSegmentDuration", 10).toInt();
    m_translateEngine = m_settings.value("translateEngine", 0).toInt();
    m_apiKey = QByteArray::fromBase64(m_settings.value("apiKey").toByteArray());
    m_baiduAppId = m_settings.value("baiduAppId").toString();
    m_apiUrl = m_settings.value("apiUrl").toString();
    m_subtitleStyle = m_settings.value("subtitleStyle", 0).toInt();
    m_keepWav = m_settings.value("keepWav", true).toBool();
    m_keepOriginalSrt = m_settings.value("keepOriginalSrt", true).toBool();
    m_keepTranslatedSrt = m_settings.value("keepTranslatedSrt", true).toBool();
    m_defaultFontSize = m_settings.value("defaultFontSize", 20).toInt();
    m_defaultFontColor = m_settings.value("defaultFontColor", "#FFFFFF").toString();
    m_defaultBorderColor = m_settings.value("defaultBorderColor", "#000000").toString();
    m_defaultBorderWidth = m_settings.value("defaultBorderWidth", 2).toInt();
    m_useGpuAccel = m_settings.value("useGpuAccel", false).toBool();
    m_libreTranslateUrl = m_settings.value("libreTranslateUrl", "http://localhost:5000").toString();
    m_inputPath = m_settings.value("inputPath").toString();
    m_inputMode = m_settings.value("inputMode", 0).toInt();
    m_recursive = m_settings.value("recursive", false).toBool();
    m_sourceLanguage = m_settings.value("sourceLanguage", "auto").toString();
    m_targetLanguage = m_settings.value("targetLanguage", "zh").toString();
    m_translateMusic = m_settings.value("translateMusic", false).toBool();
    m_outputMode = m_settings.value("outputMode", 0).toInt();
    m_outputDir = m_settings.value("outputDir").toString();
    m_enableAudioExtraction = m_settings.value("enableAudioExtraction", true).toBool();
    m_enableTranscribe = m_settings.value("enableTranscribe", true).toBool();
    m_enableTranslate = m_settings.value("enableTranslate", true).toBool();
    m_enableBurnSubtitle = m_settings.value("enableBurnSubtitle", true).toBool();

    if (!m_ffmpegPath.isEmpty() && isFFmpegAvailable(m_ffmpegPath, m_logger)) {
        m_gpuAccelInfo = "GPU 加速: " + FFmpegService::hardwareAccelName(m_ffmpegPath);
    } else {
        m_gpuAccelInfo = "GPU 加速: 需先配置 FFmpeg";
    }

    if (m_apiUrl.isEmpty()) {
        updateApiUrlForEngine(m_translateEngine);
    }

    detectTools();

    emit ffmpegPathChanged();
    emit whisperPathChanged();
    emit whisperModelChanged();
    emit whisperModelDirChanged();
    emit localModelPathChanged();
    emit audioSegmentDurationChanged();
    emit translateEngineChanged();
    emit apiKeyChanged();
    emit baiduAppIdChanged();
    emit apiUrlChanged();
    emit subtitleStyleChanged();
    emit keepWavChanged();
    emit keepOriginalSrtChanged();
    emit keepTranslatedSrtChanged();
    emit defaultFontSizeChanged();
    emit defaultFontColorChanged();
    emit defaultBorderColorChanged();
    emit defaultBorderWidthChanged();
    emit useGpuAccelChanged();
    emit gpuAccelInfoChanged();
    emit libreTranslateUrlChanged();
    emit libreTranslateStatusChanged();
    emit inputPathChanged();
    emit inputModeChanged();
    emit recursiveChanged();
    emit sourceLanguageChanged();
    emit targetLanguageChanged();
    emit translateMusicChanged();
    emit outputModeChanged();
    emit outputDirChanged();
    emit enableAudioExtractionChanged();
    emit enableTranscribeChanged();
    emit enableTranslateChanged();
    emit enableBurnSubtitleChanged();
}
void VideoSubtitleSettings::saveSettings()
{
    m_settings.setValue("ffmpegPath", m_ffmpegPath);
    m_settings.setValue("whisperPath", m_whisperPath);
    m_settings.setValue("whisperModel", m_whisperModel);
    m_settings.setValue("localModelPath", m_localModelPath);
    m_settings.setValue("whisperModelDir", m_whisperModelDir);
    m_settings.setValue("audioSegmentDuration", m_audioSegmentDuration);
    m_settings.setValue("translateEngine", m_translateEngine);
    m_settings.setValue("apiKey", QString(m_apiKey.toUtf8().toBase64()));
    m_settings.setValue("baiduAppId", m_baiduAppId);
    m_settings.setValue("apiUrl", m_apiUrl);
    m_settings.setValue("subtitleStyle", m_subtitleStyle);
    m_settings.setValue("keepWav", m_keepWav);
    m_settings.setValue("keepOriginalSrt", m_keepOriginalSrt);
    m_settings.setValue("keepTranslatedSrt", m_keepTranslatedSrt);
    m_settings.setValue("defaultFontSize", m_defaultFontSize);
    m_settings.setValue("defaultFontColor", m_defaultFontColor);
    m_settings.setValue("defaultBorderColor", m_defaultBorderColor);
    m_settings.setValue("defaultBorderWidth", m_defaultBorderWidth);
    m_settings.setValue("useGpuAccel", m_useGpuAccel);
    m_settings.setValue("libreTranslateUrl", m_libreTranslateUrl);
    m_settings.setValue("inputPath", m_inputPath);
    m_settings.setValue("inputMode", m_inputMode);
    m_settings.setValue("recursive", m_recursive);
    m_settings.setValue("sourceLanguage", m_sourceLanguage);
    m_settings.setValue("targetLanguage", m_targetLanguage);
    m_settings.setValue("translateMusic", m_translateMusic);
    m_settings.setValue("outputMode", m_outputMode);
    m_settings.setValue("outputDir", m_outputDir);
    m_settings.setValue("enableAudioExtraction", m_enableAudioExtraction);
    m_settings.setValue("enableTranscribe", m_enableTranscribe);
    m_settings.setValue("enableTranslate", m_enableTranslate);
    m_settings.setValue("enableBurnSubtitle", m_enableBurnSubtitle);
    m_settings.sync();

    emit settingsChanged();
}
void VideoSubtitleSettings::resetDefaults()
{
    m_ffmpegPath.clear();
    m_ffmpegStatus.clear();
    m_whisperPath.clear();
    m_whisperStatus.clear();
    m_whisperModel = 3;
    m_whisperModelDir = QCoreApplication::applicationDirPath() + "/plugins/videosubtitle";
    m_localModelPath.clear();
    m_audioSegmentDuration = 10;
    m_translateEngine = 0;
    m_apiKey.clear();
    m_baiduAppId.clear();
    m_apiUrl = defaultApiUrl(0);
    m_subtitleStyle = 0;
    m_keepWav = true;
    m_keepOriginalSrt = true;
    m_keepTranslatedSrt = true;
    m_defaultFontSize = 20;
    m_defaultFontColor = "#FFFFFF";
    m_defaultBorderColor = "#000000";
    m_defaultBorderWidth = 2;
    m_useGpuAccel = false;
    m_gpuAccelInfo = "GPU 加速: 需先配置 FFmpeg";
    m_libreTranslateUrl = "http://localhost:5000";
    m_libreTranslateStatus.clear();
    m_inputPath.clear();
    m_inputMode = 0;
    m_recursive = false;
    m_sourceLanguage = "auto";
    m_targetLanguage = "zh";
    m_translateMusic = false;
    m_outputMode = 0;
    m_outputDir.clear();
    m_enableAudioExtraction = true;
    m_enableTranscribe = true;
    m_enableTranslate = true;
    m_enableBurnSubtitle = true;

    detectTools();

    emit ffmpegPathChanged();
    emit ffmpegStatusChanged();
    emit whisperPathChanged();
    emit whisperStatusChanged();
    emit whisperModelChanged();
    emit whisperModelDirChanged();
    emit localModelPathChanged();
    emit audioSegmentDurationChanged();
    emit translateEngineChanged();
    emit apiKeyChanged();
    emit baiduAppIdChanged();
    emit apiUrlChanged();
    emit subtitleStyleChanged();
    emit keepWavChanged();
    emit keepOriginalSrtChanged();
    emit keepTranslatedSrtChanged();
    emit defaultFontSizeChanged();
    emit defaultFontColorChanged();
    emit defaultBorderColorChanged();
    emit defaultBorderWidthChanged();
    emit useGpuAccelChanged();
    emit gpuAccelInfoChanged();
    emit libreTranslateUrlChanged();
    emit libreTranslateStatusChanged();
    emit inputPathChanged();
    emit inputModeChanged();
    emit recursiveChanged();
    emit sourceLanguageChanged();
    emit targetLanguageChanged();
    emit translateMusicChanged();
    emit outputModeChanged();
    emit outputDirChanged();
    emit enableAudioExtractionChanged();
    emit enableTranscribeChanged();
    emit enableTranslateChanged();
    emit enableBurnSubtitleChanged();
}
void VideoSubtitleSettings::testFfmpeg()
{
    if (isFFmpegAvailable(m_ffmpegPath, m_logger)) {
        QString ver = ffmpegVersion(m_ffmpegPath);
        m_ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
    } else {
        m_ffmpegStatus = "无法找到 FFmpeg";
    }
    emit ffmpegStatusChanged();
}
void VideoSubtitleSettings::testWhisper()
{
    if (!m_whisperPath.isEmpty() && WhisperService::isWhisperAvailable(m_whisperPath, m_logger)) {
        m_whisperStatus = "已检测到 whisper.cpp";
    } else {
        m_whisperStatus = m_whisperPath.isEmpty() ? "未配置" : "无法找到 whisper.cpp（请检查运行时 DLL）";
    }
    emit whisperStatusChanged();
}
void VideoSubtitleSettings::testApiConnection()
{
    switch (m_translateEngine) {
    case 0:
        testBaiduConnection();
        break;
    case 1:
        testLibreTranslateConnection();
        break;
    default:
        m_apiTestResult = "不支持的翻译引擎";
        emit apiTestResultChanged();
        break;
    }
}
void VideoSubtitleSettings::testBaiduConnection()
{
    if (m_apiKey.isEmpty()) {
        m_apiTestResult = "请先输入百度 Secret Key";
        emit apiTestResultChanged();
        return;
    }

    if (m_baiduAppId.isEmpty()) {
        m_apiTestResult = "请先输入百度 App ID";
        emit apiTestResultChanged();
        return;
    }

    m_apiTesting = true;
    emit apiTestingChanged();

    QString query = "Hello";
    QString salt = QString::number(QRandomGenerator::global()->generate());
    QString signStr = m_baiduAppId + query + salt + m_apiKey;
    QString sign = QCryptographicHash::hash(signStr.toUtf8(), QCryptographicHash::Md5).toHex().toLower();

    QString encodedQ = QString::fromUtf8(QUrl::toPercentEncoding(query));
    QString fullUrl = QString("http://api.fanyi.baidu.com/api/trans/vip/translate"
                              "?q=%1&from=auto&to=zh&appid=%2&salt=%3&sign=%4")
                          .arg(encodedQ, m_baiduAppId, salt, sign);

    m_logger->info(QString("测试百度翻译 API 连接"));
    m_logger->info(QString("  appid=%1").arg(m_baiduAppId));
    m_logger->info(QString("  q=%1").arg(query));
    m_logger->info(QString("  salt=%1").arg(salt));
    m_logger->info(QString("  密钥(secretKey)=%1").arg(m_apiKey));
    m_logger->info(QString("  拼接串 signStr=%1").arg(signStr));
    m_logger->info(QString("  计算 sign=%1").arg(sign));
    m_logger->info(QString("  完整URL=%1").arg(fullUrl));

    m_logger->restRequest("GET", fullUrl);

    QNetworkRequest request{QUrl(fullUrl)};
    m_testReply = m_networkManager->get(request);
    connect(m_testReply, &QNetworkReply::finished, this, [this]() {
        m_apiTesting = false;
        emit apiTestingChanged();

        if (m_testReply->error() == QNetworkReply::NoError) {
            QByteArray data = m_testReply->readAll();
            m_logger->restResponse(m_testReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                                       QString::fromUtf8(data));
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject() && doc.object().contains("trans_result")) {
                m_apiTestResult = "连接正常";
                m_logger->info("百度翻译 API 测试成功");
            } else if (doc.isObject() && doc.object().contains("error_code")) {
                QString errMsg = doc.object()["error_msg"].toString();
                m_apiTestResult = "API 错误: " + errMsg;
                m_logger->error("百度翻译 API 测试失败: " + errMsg);
            } else {
                m_apiTestResult = "响应格式异常";
                m_logger->error("百度翻译 API 返回格式异常");
            }
        } else {
            m_apiTestResult = "连接失败: " + m_testReply->errorString();
            m_logger->error("百度翻译请求失败: " + m_testReply->errorString());
        }
        m_testReply->deleteLater();
        m_testReply = nullptr;
        emit apiTestResultChanged();
    });
}
void VideoSubtitleSettings::testLibreTranslateConnection()
{
    if (m_libreTranslateUrl.isEmpty()) {
        m_libreTranslateStatus = "请先输入服务地址";
        emit libreTranslateStatusChanged();
        return;
    }

    m_apiTesting = true;
    emit apiTestingChanged();

    QString baseUrl = m_libreTranslateUrl;
    if (baseUrl.endsWith('/'))
        baseUrl.chop(1);

    QString fullUrl = baseUrl + "/spec";

    m_logger->info(QString("测试 LibreTranslate 连接: %1").arg(fullUrl));

    QNetworkRequest request{QUrl(fullUrl)};
    request.setTransferTimeout(5000);
    m_testReply = m_networkManager->get(request);
    connect(m_testReply, &QNetworkReply::finished, this, [this]() {
        m_apiTesting = false;
        emit apiTestingChanged();

        if (m_testReply->error() == QNetworkReply::NoError) {
            int httpStatus = m_testReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (httpStatus == 200) {
                m_libreTranslateStatus = "连接正常";
                m_apiTestResult = "连接正常";
                m_logger->info("LibreTranslate 连接测试成功");
            } else {
                m_libreTranslateStatus = QString("服务返回 HTTP %1").arg(httpStatus);
                m_apiTestResult = m_libreTranslateStatus;
                m_logger->error("LibreTranslate 返回异常: HTTP " + QString::number(httpStatus));
            }
        } else {
            QString errStr = m_testReply->errorString();
            m_libreTranslateStatus = "连接失败: " + errStr;
            m_apiTestResult = m_libreTranslateStatus;
            m_logger->error("LibreTranslate 连接失败: " + errStr);
        }
        m_testReply->deleteLater();
        m_testReply = nullptr;
        emit libreTranslateStatusChanged();
        emit apiTestResultChanged();
    });
}
QString VideoSubtitleSettings::whisperModelPath() const
{
    if (!m_localModelPath.isEmpty() && QFileInfo::exists(m_localModelPath))
        return m_localModelPath;
    QStringList modelFiles = {"ggml-tiny.bin", "ggml-base.bin", "ggml-small.bin",
                              "ggml-medium.bin", "ggml-large-v3.bin"};
    if (m_whisperModel >= 0 && m_whisperModel < modelFiles.size())
        return m_whisperModelDir + "/" + modelFiles[m_whisperModel];
    return {};
}
bool VideoSubtitleSettings::isModelDownloaded(int modelIndex) const
{
    if (modelIndex < 0 || modelIndex >= MODEL_INFOS.size()) return false;
    QString filePath = m_whisperModelDir + "/" + MODEL_INFOS[modelIndex].fileName;
    return QFileInfo::exists(filePath);
}
void VideoSubtitleSettings::deleteModel(int modelIndex)
{
    if (modelIndex < 0 || modelIndex >= MODEL_INFOS.size()) return;
    QString filePath = m_whisperModelDir + "/" + MODEL_INFOS[modelIndex].fileName;
    QFile::remove(filePath);
    emit availableModelsChanged();
}
QString VideoSubtitleSettings::modelFileName(int modelIndex) const
{
    if (modelIndex < 0 || modelIndex >= MODEL_INFOS.size()) return QString();
    return MODEL_INFOS[modelIndex].fileName;
}
qint64 VideoSubtitleSettings::modelFileSize(int modelIndex) const
{
    if (modelIndex < 0 || modelIndex >= MODEL_INFOS.size()) return 0;
    return MODEL_INFOS[modelIndex].fileSize;
}

// ── Private ──
void VideoSubtitleSettings::detectTools()
{
    bool changed = false;

    auto findBundled = [](const QString &fileName) -> QString {
        QString basePath = QCoreApplication::applicationDirPath() + "/plugins/videosubtitle";
        QDirIterator it(basePath, QStringList() << fileName, QDir::Files, QDirIterator::Subdirectories);
        if (it.hasNext()) {
            it.next();
            return it.filePath();
        }
        return QString();
    };

    if (!m_ffmpegPath.isEmpty()) {
        if (isFFmpegAvailable(m_ffmpegPath, m_logger)) {
            QString ver = ffmpegVersion(m_ffmpegPath);
            m_ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
        } else {
            m_ffmpegStatus = "无法找到 FFmpeg";
            m_logger->warn("ini 配置的 FFmpeg 路径不可用: " + m_ffmpegPath);
        }
        emit ffmpegStatusChanged();
    } else {
        QString found = findBundled("ffmpeg.exe");
        if (!found.isEmpty()) {
            m_ffmpegPath = found;
            if (isFFmpegAvailable(m_ffmpegPath, m_logger)) {
                QString ver = ffmpegVersion(m_ffmpegPath);
                m_ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
            } else {
                m_ffmpegStatus = "无法找到 FFmpeg";
            }
            changed = true;
            m_logger->info("自动检测到自带的 FFmpeg: " + m_ffmpegPath);
        }
        emit ffmpegStatusChanged();
    }

    if (!m_ffmpegPath.isEmpty() && isFFmpegAvailable(m_ffmpegPath, m_logger)) {
        m_gpuAccelInfo = "GPU 加速: " + FFmpegService::hardwareAccelName(m_ffmpegPath);
    } else {
        m_gpuAccelInfo = "GPU 加速: 需先配置 FFmpeg";
    }

    if (!m_whisperPath.isEmpty()) {
        if (QFileInfo::exists(m_whisperPath)) {
            m_whisperStatus = "已找到 whisper.cpp";
        } else {
            m_whisperStatus = "无法找到 whisper.cpp";
            m_logger->warn("ini 配置的 Whisper 路径不可用: " + m_whisperPath);
        }
        emit whisperStatusChanged();
    } else {
        QString found = findBundled("whisper-cli.exe");
        if (!found.isEmpty()) {
            m_whisperPath = found;
            m_whisperStatus = "已找到 whisper.cpp";
            changed = true;
            m_logger->info("自动检测到自带的 Whisper: " + m_whisperPath);
        }
        emit whisperStatusChanged();
    }

    if (changed) {
        saveSettings();
    }
}
void VideoSubtitleSettings::updateApiUrlForEngine(int engine)
{
    m_apiUrl = defaultApiUrl(engine);
}
QString VideoSubtitleSettings::defaultApiUrl(int engine) const
{
    switch (engine) {
    case 0: return "http://api.fanyi.baidu.com/api/trans/vip/translate";
    case 1: return "http://localhost:5000";
    default: return QString();
    }
}
