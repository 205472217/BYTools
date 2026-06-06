#include "VideoSubtitleSettings.h"
#include "PluginLogger.h"
#include "FFmpegService.h"
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

VideoSubtitleSettings::VideoSubtitleSettings(QObject *parent)
    : QObject(parent)
    , m_settings(configPath(), QSettings::IniFormat)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_testReply(nullptr)
{
    m_settings.beginGroup("VideoSubtitle");
    m_whisperModelDir = QCoreApplication::applicationDirPath() + "/plugins/videosubtitle";
    loadSettings();
    PluginLogger::info("插件设置已加载");
}

// Getters
QString VideoSubtitleSettings::ffmpegPath() const { return m_ffmpegPath; }
QString VideoSubtitleSettings::ffmpegStatus() const { return m_ffmpegStatus; }
QString VideoSubtitleSettings::whisperPath() const { return m_whisperPath; }
QString VideoSubtitleSettings::whisperStatus() const { return m_whisperStatus; }
int VideoSubtitleSettings::whisperModel() const { return m_whisperModel; }
QString VideoSubtitleSettings::whisperModelDir() const { return m_whisperModelDir; }
QString VideoSubtitleSettings::localModelPath() const { return m_localModelPath; }
int VideoSubtitleSettings::audioSegmentDuration() const { return m_audioSegmentDuration; }
int VideoSubtitleSettings::subtitleStyle() const { return m_subtitleStyle; }
bool VideoSubtitleSettings::keepWav() const { return m_keepWav; }
bool VideoSubtitleSettings::keepOriginalSrt() const { return m_keepOriginalSrt; }
bool VideoSubtitleSettings::keepTranslatedSrt() const { return m_keepTranslatedSrt; }

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
int VideoSubtitleSettings::translateEngine() const { return m_translateEngine; }
QString VideoSubtitleSettings::apiKey() const { return m_apiKey; }
QString VideoSubtitleSettings::baiduAppId() const { return m_baiduAppId; }
QString VideoSubtitleSettings::apiUrl() const { return m_apiUrl; }
QString VideoSubtitleSettings::apiTestResult() const { return m_apiTestResult; }
bool VideoSubtitleSettings::apiTesting() const { return m_apiTesting; }
int VideoSubtitleSettings::defaultFontSize() const { return m_defaultFontSize; }
QString VideoSubtitleSettings::defaultFontColor() const { return m_defaultFontColor; }
QString VideoSubtitleSettings::defaultBorderColor() const { return m_defaultBorderColor; }
int VideoSubtitleSettings::defaultBorderWidth() const { return m_defaultBorderWidth; }

// Setters
void VideoSubtitleSettings::setFfmpegPath(const QString &path)
{
    if (m_ffmpegPath != path) {
        m_ffmpegPath = path;
        emit ffmpegPathChanged();
        // Auto-detect version
        if (FFmpegService::isFFmpegAvailable(path)) {
            QString ver = FFmpegService::ffmpegVersion(path);
            m_ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
        } else {
            m_ffmpegStatus = path.isEmpty() ? "未配置" : "无法找到 FFmpeg";
        }
        emit ffmpegStatusChanged();
    }
}

void VideoSubtitleSettings::setWhisperPath(const QString &path)
{
    if (m_whisperPath != path) {
        m_whisperPath = path;
        emit whisperPathChanged();
        // Auto-detect
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

// Actions
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
}

void VideoSubtitleSettings::testFfmpeg()
{
    if (FFmpegService::isFFmpegAvailable(m_ffmpegPath)) {
        QString ver = FFmpegService::ffmpegVersion(m_ffmpegPath);
        m_ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
    } else {
        m_ffmpegStatus = "无法找到 FFmpeg";
    }
    emit ffmpegStatusChanged();
}

void VideoSubtitleSettings::testWhisper()
{
    if (!m_whisperPath.isEmpty() && WhisperService::isWhisperAvailable(m_whisperPath)) {
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
    default:
        m_apiTestResult = "不支持的翻译引擎";
        emit apiTestResultChanged();
        break;
    }
}

QStringList VideoSubtitleSettings::translateEngineNames() const
{
    // [extension]: 后续新增翻译引擎只需在此追加名称, 如 << "DeepL 翻译", "Google 翻译"
    return {"百度翻译"};
}

// --- Engine-specific test methods ---
// [extension]: 新增翻译引擎时在此下方添加对应的 testXxxConnection() 方法,
//              并在 testApiConnection() 的 switch 中注册分发。

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

    // Baidu Translate 签名规则: appid + q + salt + 密钥
    // 参数全部放入 URL（匹配已验证可用的 MFC 代码方式）
    QString query = "Hello";
    QString salt = QString::number(QRandomGenerator::global()->generate());
    QString signStr = m_baiduAppId + query + salt + m_apiKey;
    QString sign = QCryptographicHash::hash(signStr.toUtf8(), QCryptographicHash::Md5).toHex().toLower();

    // 对 q 做 URL encode（拼接 sign 时用的是原始 q）
    QString encodedQ = QString::fromUtf8(QUrl::toPercentEncoding(query));
    QString fullUrl = QString("http://api.fanyi.baidu.com/api/trans/vip/translate"
                              "?q=%1&from=auto&to=zh&appid=%2&salt=%3&sign=%4")
                          .arg(encodedQ, m_baiduAppId, salt, sign);

    // 日志记录完整签名信息（调试阶段全部打印）
    PluginLogger::info(QString("测试百度翻译 API 连接"));
    PluginLogger::info(QString("  appid=%1").arg(m_baiduAppId));
    PluginLogger::info(QString("  q=%1").arg(query));
    PluginLogger::info(QString("  salt=%1").arg(salt));
    PluginLogger::info(QString("  密钥(secretKey)=%1").arg(m_apiKey));
    PluginLogger::info(QString("  拼接串 signStr=%1").arg(signStr));
    PluginLogger::info(QString("  计算 sign=%1").arg(sign));
    PluginLogger::info(QString("  完整URL=%1").arg(fullUrl));

    // 使用 GET 请求（与工作代码的 Http_POST 空 body 方式等效）
    PluginLogger::restRequest("GET", fullUrl);

    QNetworkRequest request{QUrl(fullUrl)};
    m_testReply = m_networkManager->get(request);
    connect(m_testReply, &QNetworkReply::finished, this, [this]() {
        m_apiTesting = false;
        emit apiTestingChanged();

        if (m_testReply->error() == QNetworkReply::NoError) {
            // Check response for valid translation result
            QByteArray data = m_testReply->readAll();
            PluginLogger::restResponse(m_testReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                                       QString::fromUtf8(data));
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject() && doc.object().contains("trans_result")) {
                m_apiTestResult = "连接正常";
                PluginLogger::info("百度翻译 API 测试成功");
            } else if (doc.isObject() && doc.object().contains("error_code")) {
                QString errMsg = doc.object()["error_msg"].toString();
                m_apiTestResult = "API 错误: " + errMsg;
                PluginLogger::error("百度翻译 API 测试失败: " + errMsg);
            } else {
                m_apiTestResult = "响应格式异常";
                PluginLogger::error("百度翻译 API 返回格式异常");
            }
        } else {
            m_apiTestResult = "连接失败: " + m_testReply->errorString();
            PluginLogger::error("百度翻译请求失败: " + m_testReply->errorString());
        }
        m_testReply->deleteLater();
        m_testReply = nullptr;
        emit apiTestResultChanged();
    });
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

// Private
void VideoSubtitleSettings::detectTools()
{
    bool changed = false;

    // ---------- Helper: recursively find a bundled tool under exe同级/plugins/videosubtitle/ ----------
    auto findBundled = [](const QString &fileName) -> QString {
        QString basePath = QCoreApplication::applicationDirPath() + "/plugins/videosubtitle";
        QDirIterator it(basePath, QStringList() << fileName, QDir::Files, QDirIterator::Subdirectories);
        if (it.hasNext()) {
            it.next();
            return it.filePath();
        }
        return QString();
    };

    // ---------- FFmpeg ----------
    if (!m_ffmpegPath.isEmpty()) {
        // ini 有路径，以 ini 为准，验证可用性但不回退到自动检测
        if (FFmpegService::isFFmpegAvailable(m_ffmpegPath)) {
            QString ver = FFmpegService::ffmpegVersion(m_ffmpegPath);
            m_ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
        } else {
            m_ffmpegStatus = "无法找到 FFmpeg";
            PluginLogger::warn("ini 配置的 FFmpeg 路径不可用: " + m_ffmpegPath);
        }
        emit ffmpegStatusChanged();
    } else {
        // ini 无路径，exe同级/plugins/videosubtitle/ 下递归查找 ffmpeg.exe
        QString found = findBundled("ffmpeg.exe");
        if (!found.isEmpty()) {
            m_ffmpegPath = found;
            if (FFmpegService::isFFmpegAvailable(m_ffmpegPath)) {
                QString ver = FFmpegService::ffmpegVersion(m_ffmpegPath);
                m_ffmpegStatus = ver.isEmpty() ? "已找到 FFmpeg" : ("已检测到 FFmpeg " + ver);
            } else {
                m_ffmpegStatus = "无法找到 FFmpeg";
            }
            changed = true;
            PluginLogger::info("自动检测到自带的 FFmpeg: " + m_ffmpegPath);
        }
        emit ffmpegStatusChanged();
    }

    // ---------- Whisper ----------
    // 注意: 只检查文件是否存在，不启动进程验证。
    // whisper-cli.exe 依赖 whisper.dll / ggml.dll，启动时会触发 Windows DLL 加载，
    // BYTools 不应因外部工具的运行时环境而报错。
    if (!m_whisperPath.isEmpty()) {
        // ini 有路径，以 ini 为准
        if (QFileInfo::exists(m_whisperPath)) {
            m_whisperStatus = "已找到 whisper.cpp";
        } else {
            m_whisperStatus = "无法找到 whisper.cpp";
            PluginLogger::warn("ini 配置的 Whisper 路径不可用: " + m_whisperPath);
        }
        emit whisperStatusChanged();
    } else {
        // ini 无路径，exe同级/plugins/videosubtitle/ 下递归查找 whisper-cli.exe
        QString found = findBundled("whisper-cli.exe");
        if (!found.isEmpty()) {
            m_whisperPath = found;
            m_whisperStatus = "已找到 whisper.cpp";
            changed = true;
            PluginLogger::info("自动检测到自带的 Whisper: " + m_whisperPath);
        }
        emit whisperStatusChanged();
    }

    // 自动保存检测到的路径，确保 Controller 可以读取到
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
    default: return QString();
    }
}
