#include "TranslateService.h"
#include "SubtitleService.h"
#include "PluginLogger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QRandomGenerator>

static const int BATCH_SIZE = 50;

TranslateService::TranslateService(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_cancelled(false)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &TranslateService::onReplyFinished);
}

void TranslateService::startTranslate(const QString &inputSrtPath,
                                       const QString &outputSrtPath,
                                       int engine,
                                       const QString &apiKey,
                                       const QString &apiUrl,
                                       const QString &targetLang,
                                       const QString &baiduAppId,
                                       bool bilingual)
{
    if (m_cancelled) return;

    m_cancelled = false;
    m_inputSrtPath = inputSrtPath;
    m_outputSrtPath = outputSrtPath;
    m_currentEngine = engine;
    m_currentApiKey = apiKey;
    m_currentApiUrl = apiUrl;
    m_currentTargetLang = targetLang;
    m_bilingual = bilingual;

    // Parse SRT
    QList<SubtitleService::SubtitleEntry> entries = SubtitleService::parseSrt(inputSrtPath);
    if (entries.isEmpty()) {
        emit finished(false, outputSrtPath, "Failed to parse SRT file or file is empty");
        return;
    }

    // Split into batches
    m_totalBatches = (entries.size() + BATCH_SIZE - 1) / BATCH_SIZE;
    m_completedBatches = 0;
    m_translatedTexts = QJsonArray();

    // Pre-allocate result array
    for (int i = 0; i < entries.size(); ++i) {
        m_translatedTexts.append("");
    }

    // Start translating first batch
    if (m_totalBatches > 0) {
        QJsonArray batchTexts;
        int end = qMin(BATCH_SIZE, entries.size());
        for (int i = 0; i < end; ++i) {
            batchTexts.append(entries.at(i).originalText);
        }
        translateBatch(batchTexts, engine, apiKey, apiUrl, targetLang, 0, m_totalBatches, baiduAppId);
    }
}

void TranslateService::cancel()
{
    m_cancelled = true;
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply = nullptr;
    }
}

void TranslateService::translateBatch(const QJsonArray &texts, int engine,
                                       const QString &apiKey, const QString &apiUrl,
                                       const QString &targetLang, int batchIndex, int totalBatches,
                                       const QString &baiduAppId)
{
    switch (engine) {
    case 0: { // Baidu Translate
        // 参数全部放入 URL（匹配已验证可用的 MFC 代码方式）
        QString fullUrl = buildBaiduUrl(texts, targetLang, apiKey, baiduAppId);
        m_currentBatchIndex = batchIndex;

        PluginLogger::restRequest("GET", fullUrl);

        QNetworkRequest request{QUrl(fullUrl)};
        m_currentReply = m_networkManager->get(request);
        break;
    }
    default:
        emit finished(false, m_outputSrtPath, "Unknown translation engine");
        return;
    }
}

void TranslateService::onReplyFinished(QNetworkReply *reply)
{
    if (m_cancelled) {
        reply->deleteLater();
        return;
    }

    m_currentReply = nullptr;
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        QString errStr = reply->errorString();
        reply->deleteLater();
        PluginLogger::restResponse(httpStatus, "网络错误: " + errStr);
        emit finished(false, m_outputSrtPath, "Network error: " + errStr);
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    PluginLogger::restResponse(httpStatus, QString::fromUtf8(responseData));
    QJsonObject root = doc.object();

    // 检查 API 错误（百度返回 error_code 时为失败响应）
    if (root.contains("error_code")) {
        QString errCode = root["error_code"].toString();
        QString errMsg  = root["error_msg"].toString();
        PluginLogger::error(QString("百度翻译 API 错误 [%1]: %2").arg(errCode, errMsg));
        emit finished(false, m_outputSrtPath, QString("百度翻译错误 [%1]: %2").arg(errCode, errMsg));
        return;
    }

    QStringList translatedTexts;

    switch (m_currentEngine) {
    case 0: { // Baidu Translate
        QJsonArray transResult = root["trans_result"].toArray();
        for (const QJsonValue &v : transResult) {
            translatedTexts.append(v.toObject()["dst"].toString());
        }
        break;
    }
    default: {
        emit finished(false, m_outputSrtPath, "Unknown translation engine response");
        return;
    }
    }

    if (translatedTexts.isEmpty()) {
        PluginLogger::error("翻译返回结果为空");
        emit finished(false, m_outputSrtPath, "No translation result returned");
        return;
    }

    // Store results in correct positions
    int offset = m_currentBatchIndex * BATCH_SIZE;
    for (int i = 0; i < translatedTexts.size(); ++i) {
        if (offset + i < m_translatedTexts.size()) {
            m_translatedTexts[offset + i] = translatedTexts.at(i);
        }
    }

    m_completedBatches++;
    emit progress(static_cast<double>(m_completedBatches) / m_totalBatches);

    // Check if all batches done
    if (m_completedBatches >= m_totalBatches) {
        // Re-parse original SRT to preserve timing info
        QList<SubtitleService::SubtitleEntry> originalEntries =
            SubtitleService::parseSrt(m_inputSrtPath);

        // Merge translated texts with original timing
        QList<SubtitleService::SubtitleEntry> outputEntries;
        int count = qMin(originalEntries.size(), m_translatedTexts.size());
        for (int i = 0; i < count; ++i) {
            SubtitleService::SubtitleEntry entry = originalEntries.at(i);
            entry.translatedText = m_translatedTexts.at(i).toString();
            outputEntries.append(entry);
        }

        // Write SRT respecting the bilingual setting
        bool ok = SubtitleService::writeSrt(m_outputSrtPath, outputEntries, m_bilingual);
        emit finished(ok, m_outputSrtPath, ok ? "" : "Failed to write SRT file");
    }
}

QString TranslateService::buildBaiduUrl(const QJsonArray &texts, const QString &targetLang,
                                         const QString &secretKey, const QString &appId)
{
    // Baidu Translate 签名规则: appid + q + salt + 密钥
    // 参数全部放入 URL（匹配已验证可用的 MFC 代码方式）
    QStringList textList;
    for (const QJsonValue &v : texts) {
        textList.append(v.toString());
    }
    QString query = textList.join("\n");
    QString salt = QString::number(QRandomGenerator::global()->generate());

    QString signStr = appId + query + salt + secretKey;
    QString sign = QCryptographicHash::hash(signStr.toUtf8(), QCryptographicHash::Md5).toHex().toLower();

    // 对 q 做 URL encode（拼接 sign 时用的是原始 q）
    QString encodedQ = QString::fromUtf8(QUrl::toPercentEncoding(query));
    QString url = QString("http://api.fanyi.baidu.com/api/trans/vip/translate"
                          "?q=%1&from=auto&to=%2&appid=%3&salt=%4&sign=%5")
                      .arg(encodedQ, targetLang, appId, salt, sign);
    return url;
}
