#include "VideoSubtitleSettings.h"
#include "VideoSubtitlePlugin.h"
#include "SettingsHelper.h"
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
#include <QtConcurrent/QtConcurrentRun>

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
    , m_networkManager(new QNetworkAccessManager(this))
    , m_testReply(nullptr)
    , m_libreTranslateUrl("http://localhost:5000")
{
    m_whisperModelDir = QCoreApplication::applicationDirPath() + "/plugins/videosubtitle";

    m_detectWatcher = new QFutureWatcher<ToolsDetectResult>(this);
    connect(m_detectWatcher, &QFutureWatcher<ToolsDetectResult>::finished,
            this, &VideoSubtitleSettings::onToolsDetectionFinished);

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
        if (!path.isEmpty()) {
            m_ffmpegDetecting = true;
            m_ffmpegStatus = "检测中...";
            m_gpuAccelInfo = "检测中...";
            startAsyncDetection();
        } else {
            m_ffmpegDetecting = false;
            m_ffmpegStatus = "未配置";
            m_gpuAccelInfo = "GPU 加速: 需先配置 FFmpeg";
        }
        saveSettings();
    }
}
void VideoSubtitleSettings::setWhisperPath(const QString &path)
{
    if (m_whisperPath != path) {
        m_whisperPath = path;
        if (!path.isEmpty() && QFileInfo::exists(path)) {
            m_whisperStatus = "已检测到 whisper.cpp";
        } else {
            m_whisperStatus = path.isEmpty() ? "未配置" : "无法找到 whisper.cpp";
        }
        saveSettings();
    }
}
void VideoSubtitleSettings::setWhisperModel(int model)
{
    if (m_whisperModel != model && model >= 0 && model < MODEL_INFOS.size()) {
        m_whisperModel = model;
        saveSettings();
    }
}
void VideoSubtitleSettings::setWhisperModelDir(const QString &path)
{
    if (m_whisperModelDir != path) {
        m_whisperModelDir = path;
        saveSettings();
    }
}
void VideoSubtitleSettings::setLocalModelPath(const QString &path)
{
    if (m_localModelPath != path) {
        m_localModelPath = path;
        saveSettings();
    }
}
void VideoSubtitleSettings::setAudioSegmentDuration(int seconds)
{
    if (m_audioSegmentDuration != seconds) {
        m_audioSegmentDuration = seconds;
        saveSettings();
    }
}
void VideoSubtitleSettings::setTranslateEngine(int engine)
{
    if (m_translateEngine != engine) {
        m_translateEngine = engine;
        updateApiUrlForEngine(engine);
        saveSettings();
    }
}
void VideoSubtitleSettings::setApiKey(const QString &key)
{
    if (m_apiKey != key) {
        m_apiKey = key;
        saveSettings();
    }
}
void VideoSubtitleSettings::setBaiduAppId(const QString &appId)
{
    if (m_baiduAppId != appId) {
        m_baiduAppId = appId;
        saveSettings();
    }
}
void VideoSubtitleSettings::setApiUrl(const QString &url)
{
    if (m_apiUrl != url) {
        m_apiUrl = url;
        saveSettings();
    }
}
void VideoSubtitleSettings::setSubtitleStyle(int style)
{
    if (m_subtitleStyle != style) {
        m_subtitleStyle = style;
        saveSettings();
    }
}
void VideoSubtitleSettings::setKeepWav(bool keep)
{
    if (m_keepWav != keep) {
        m_keepWav = keep;
        saveSettings();
    }
}
void VideoSubtitleSettings::setKeepOriginalSrt(bool keep)
{
    if (m_keepOriginalSrt != keep) {
        m_keepOriginalSrt = keep;
        saveSettings();
    }
}
void VideoSubtitleSettings::setKeepTranslatedSrt(bool keep)
{
    if (m_keepTranslatedSrt != keep) {
        m_keepTranslatedSrt = keep;
        saveSettings();
    }
}
void VideoSubtitleSettings::setDefaultFontSize(int size)
{
    if (m_defaultFontSize != size) {
        m_defaultFontSize = size;
        saveSettings();
    }
}
void VideoSubtitleSettings::setDefaultFontColor(const QString &color)
{
    if (m_defaultFontColor != color) {
        m_defaultFontColor = color;
        saveSettings();
    }
}
void VideoSubtitleSettings::setDefaultBorderColor(const QString &color)
{
    if (m_defaultBorderColor != color) {
        m_defaultBorderColor = color;
        saveSettings();
    }
}
void VideoSubtitleSettings::setDefaultBorderWidth(int width)
{
    if (m_defaultBorderWidth != width) {
        m_defaultBorderWidth = width;
        saveSettings();
    }
}
void VideoSubtitleSettings::setUseGpuAccel(bool enable)
{
    if (m_useGpuAccel != enable) {
        m_useGpuAccel = enable;
        saveSettings();
    }
}
void VideoSubtitleSettings::setLibreTranslateUrl(const QString &url)
{
    if (m_libreTranslateUrl != url) {
        m_libreTranslateUrl = url;
        saveSettings();
    }
}

// ── 主页面 setter ──
void VideoSubtitleSettings::setInputPath(const QString &path)
{
    if (m_inputPath != path) {
        m_inputPath = path;
        if (m_inputMode != 0) // 文件模式不持久化路径
            saveSettings();
    }
}
void VideoSubtitleSettings::setInputMode(int mode)
{
    if (m_inputMode != mode) {
        m_inputMode = mode;
        saveSettings();
    }
}
void VideoSubtitleSettings::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        saveSettings();
    }
}
void VideoSubtitleSettings::setSourceLanguage(const QString &lang)
{
    if (m_sourceLanguage != lang) {
        m_sourceLanguage = lang;
        saveSettings();
    }
}
void VideoSubtitleSettings::setTargetLanguage(const QString &lang)
{
    if (m_targetLanguage != lang) {
        m_targetLanguage = lang;
        saveSettings();
    }
}
void VideoSubtitleSettings::setTranslateMusic(bool enabled)
{
    if (m_translateMusic != enabled) {
        m_translateMusic = enabled;
        saveSettings();
    }
}
void VideoSubtitleSettings::setOutputMode(int mode)
{
    if (m_outputMode != mode) {
        m_outputMode = mode;
        saveSettings();
    }
}
void VideoSubtitleSettings::setOutputDir(const QString &dir)
{
    if (m_outputDir != dir) {
        m_outputDir = dir;
        saveSettings();
    }
}
void VideoSubtitleSettings::setEnableAudioExtraction(bool enabled)
{
    if (m_enableAudioExtraction != enabled) {
        m_enableAudioExtraction = enabled;
        saveSettings();
    }
}
void VideoSubtitleSettings::setEnableTranscribe(bool enabled)
{
    if (m_enableTranscribe != enabled) {
        m_enableTranscribe = enabled;
        saveSettings();
    }
}
void VideoSubtitleSettings::setEnableTranslate(bool enabled)
{
    if (m_enableTranslate != enabled) {
        m_enableTranslate = enabled;
        saveSettings();
    }
}
void VideoSubtitleSettings::setEnableBurnSubtitle(bool enabled)
{
    if (m_enableBurnSubtitle != enabled) {
        m_enableBurnSubtitle = enabled;
        saveSettings();
    }
}

// ── 其他功能函数 ──
void VideoSubtitleSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings(VideoSubtitlePlugin::PluginKey);

    m_ffmpegPath = s.value("ffmpegPath").toString();
    m_whisperPath = s.value("whisperPath").toString();
    m_whisperModel = s.value("whisperModel", 3).toInt();
    m_whisperModelDir = s.value("whisperModelDir", m_whisperModelDir).toString();
    m_localModelPath = s.value("localModelPath").toString();
    m_audioSegmentDuration = s.value("audioSegmentDuration", 10).toInt();
    m_translateEngine = s.value("translateEngine", 0).toInt();
    m_apiKey = QByteArray::fromBase64(s.value("apiKey").toByteArray());
    m_baiduAppId = s.value("baiduAppId").toString();
    m_apiUrl = s.value("apiUrl").toString();
    m_subtitleStyle = s.value("subtitleStyle", 0).toInt();
    m_keepWav = s.value("keepWav", true).toBool();
    m_keepOriginalSrt = s.value("keepOriginalSrt", true).toBool();
    m_keepTranslatedSrt = s.value("keepTranslatedSrt", true).toBool();
    m_defaultFontSize = s.value("defaultFontSize", 18).toInt();
    m_defaultFontColor = s.value("defaultFontColor", "#FFFFFF").toString();
    m_defaultBorderColor = s.value("defaultBorderColor", "#000000").toString();
    m_defaultBorderWidth = s.value("defaultBorderWidth", 2).toInt();
    m_useGpuAccel = s.value("useGpuAccel", false).toBool();
    m_libreTranslateUrl = s.value("libreTranslateUrl", "http://localhost:5000").toString();
    m_inputPath = s.value("inputPath").toString();
    m_inputMode = s.value("inputMode", 0).toInt();
    m_recursive = s.value("recursive", false).toBool();
    m_sourceLanguage = s.value("sourceLanguage", "auto").toString();
    m_targetLanguage = s.value("targetLanguage", "zh").toString();
    m_translateMusic = s.value("translateMusic", false).toBool();
    m_outputMode = s.value("outputMode", 0).toInt();
    m_outputDir = s.value("outputDir").toString();
    m_enableAudioExtraction = s.value("enableAudioExtraction", true).toBool();
    m_enableTranscribe = s.value("enableTranscribe", true).toBool();
    m_enableTranslate = s.value("enableTranslate", true).toBool();
    m_enableBurnSubtitle = s.value("enableBurnSubtitle", true).toBool();

    // Fast bundled scan (synchronous, just file enumeration)
    bool changed = false;
    if (m_ffmpegPath.isEmpty()) {
        QString found = findBundled("ffmpeg.exe");
        if (!found.isEmpty()) {
            m_ffmpegPath = found;
            changed = true;
        }
    }
    if (m_whisperPath.isEmpty()) {
        QString found = findBundled("whisper-cli.exe");
        if (!found.isEmpty()) {
            m_whisperPath = found;
            changed = true;
        }
    }
    if (changed)
        saveSettings();

    // Set initial detection states (ffmpeg async, whisper sync)
    if (!m_ffmpegPath.isEmpty()) {
        m_ffmpegDetecting = true;
        m_ffmpegStatus = "检测中...";
    } else {
        m_ffmpegDetecting = false;
        m_ffmpegStatus = "未配置";
    }
    if (!m_whisperPath.isEmpty()) {
        m_whisperDetecting = false;
        if (QFileInfo::exists(m_whisperPath))
            m_whisperStatus = "已找到 whisper.cpp";
        else
            m_whisperStatus = "无法找到 whisper.cpp";
    } else {
        m_whisperDetecting = false;
        m_whisperStatus = "未配置";
    }

    m_gpuAccelInfo = m_ffmpegPath.isEmpty()
        ? "GPU 加速: 需先配置 FFmpeg"
        : "检测中...";

    if (m_apiUrl.isEmpty())
        updateApiUrlForEngine(m_translateEngine);


    // Start async ffmpeg detection in background thread
    if (m_ffmpegDetecting)
        startAsyncDetection();
}
void VideoSubtitleSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings(VideoSubtitlePlugin::PluginKey);
    s.setValue("ffmpegPath", m_ffmpegPath);
    s.setValue("whisperPath", m_whisperPath);
    s.setValue("whisperModel", m_whisperModel);
    s.setValue("localModelPath", m_localModelPath);
    s.setValue("whisperModelDir", m_whisperModelDir);
    s.setValue("audioSegmentDuration", m_audioSegmentDuration);
    s.setValue("translateEngine", m_translateEngine);
    s.setValue("apiKey", QString(m_apiKey.toUtf8().toBase64()));
    s.setValue("baiduAppId", m_baiduAppId);
    s.setValue("apiUrl", m_apiUrl);
    s.setValue("subtitleStyle", m_subtitleStyle);
    s.setValue("keepWav", m_keepWav);
    s.setValue("keepOriginalSrt", m_keepOriginalSrt);
    s.setValue("keepTranslatedSrt", m_keepTranslatedSrt);
    s.setValue("defaultFontSize", m_defaultFontSize);
    s.setValue("defaultFontColor", m_defaultFontColor);
    s.setValue("defaultBorderColor", m_defaultBorderColor);
    s.setValue("defaultBorderWidth", m_defaultBorderWidth);
    s.setValue("useGpuAccel", m_useGpuAccel);
    s.setValue("libreTranslateUrl", m_libreTranslateUrl);
    s.setValue("inputPath", m_inputPath);
    s.setValue("inputMode", m_inputMode);
    s.setValue("recursive", m_recursive);
    s.setValue("sourceLanguage", m_sourceLanguage);
    s.setValue("targetLanguage", m_targetLanguage);
    s.setValue("translateMusic", m_translateMusic);
    s.setValue("outputMode", m_outputMode);
    s.setValue("outputDir", m_outputDir);
    s.setValue("enableAudioExtraction", m_enableAudioExtraction);
    s.setValue("enableTranscribe", m_enableTranscribe);
    s.setValue("enableTranslate", m_enableTranslate);
    s.setValue("enableBurnSubtitle", m_enableBurnSubtitle);
    s.sync();

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
    m_defaultFontSize = 18;
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

    // Fast bundled scan
    bool changed = false;
    QString bundledFfmpeg = findBundled("ffmpeg.exe");
    if (!bundledFfmpeg.isEmpty()) {
        m_ffmpegPath = bundledFfmpeg;
        m_ffmpegDetecting = true;
        m_ffmpegStatus = "检测中...";
        m_gpuAccelInfo = "检测中...";
        changed = true;
    } else {
        m_ffmpegDetecting = false;
    }

    QString bundledWhisper = findBundled("whisper-cli.exe");
    if (!bundledWhisper.isEmpty()) {
        m_whisperPath = bundledWhisper;
        m_whisperStatus = "已找到 whisper.cpp";
        m_whisperDetecting = false;
        changed = true;
    }

    if (changed) saveSettings();


    if (m_ffmpegDetecting)
        startAsyncDetection();
}
void VideoSubtitleSettings::testFfmpeg()
{
    if (isFFmpegAvailable(m_ffmpegPath, m_logger)) {
        QString ver = ffmpegVersion(m_ffmpegPath);
        m_ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
    } else {
        m_ffmpegStatus = "无法找到 FFmpeg";
    }
}
void VideoSubtitleSettings::testWhisper()
{
    if (!m_whisperPath.isEmpty() && WhisperService::isWhisperAvailable(m_whisperPath, m_logger)) {
        m_whisperStatus = "已检测到 whisper.cpp";
    } else {
        m_whisperStatus = m_whisperPath.isEmpty() ? "未配置" : "无法找到 whisper.cpp（请检查运行时 DLL）";
    }
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
        break;
    }
}
void VideoSubtitleSettings::testBaiduConnection()
{
    if (m_apiKey.isEmpty()) {
        m_apiTestResult = "请先输入百度 Secret Key";
        return;
    }

    if (m_baiduAppId.isEmpty()) {
        m_apiTestResult = "请先输入百度 App ID";
        return;
    }

    m_apiTesting = true;

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
    });
}
void VideoSubtitleSettings::testLibreTranslateConnection()
{
    if (m_libreTranslateUrl.isEmpty()) {
        m_libreTranslateStatus = "请先输入服务地址";
        return;
    }

    m_apiTesting = true;

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
void VideoSubtitleSettings::startAsyncDetection()
{
    if (!m_ffmpegDetecting || m_ffmpegPath.isEmpty())
        return;

    auto future = QtConcurrent::run(
        [](const QString &ffmpegPath, PluginLogger *logger) -> ToolsDetectResult {
            return runToolsDetection(ffmpegPath, logger);
        },
        m_ffmpegPath, m_logger
    );
    m_detectWatcher->setFuture(future);
}
void VideoSubtitleSettings::onToolsDetectionFinished()
{
    if (!m_detectWatcher || !m_detectWatcher->isFinished())
        return;

    auto result = m_detectWatcher->future().result();

    m_ffmpegDetecting = false;
    m_ffmpegStatus = result.ffmpegStatus;
    m_gpuAccelInfo = result.gpuAccelInfo;

}
ToolsDetectResult VideoSubtitleSettings::runToolsDetection(const QString &ffmpegPath, PluginLogger *logger)
{
    ToolsDetectResult result;

    if (ffmpegPath.isEmpty()) {
        result.ffmpegStatus = "未配置";
        result.gpuAccelInfo = "GPU 加速: 需先配置 FFmpeg";
        return result;
    }

    if (isFFmpegAvailable(ffmpegPath, logger)) {
        QString ver = ffmpegVersion(ffmpegPath);
        result.ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
    } else {
        result.ffmpegStatus = "无法找到 FFmpeg";
    }

    if (isFFmpegAvailable(ffmpegPath, logger)) {
        result.gpuAccelInfo = "GPU 加速: " + FFmpegService::hardwareAccelName(ffmpegPath);
    } else {
        result.gpuAccelInfo = "GPU 加速: 需先配置 FFmpeg";
    }

    return result;
}
QString VideoSubtitleSettings::findBundled(const QString &fileName) const
{
    QString basePath = QCoreApplication::applicationDirPath() + "/plugins/videosubtitle";
    QDirIterator it(basePath, QStringList() << fileName, QDir::Files, QDirIterator::Subdirectories);
    if (it.hasNext()) {
        it.next();
        return it.filePath();
    }
    return QString();
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
