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
#include <QRegularExpression>

// Map ISO 639-1 language codes to Baidu Translate API codes
// Baidu uses non-standard codes for some languages (e.g. jp for ja, kor for ko)
static QString toBaiduLangCode(const QString &isoCode)
{
    static const QHash<QString, QString> map = {
        // Japanese
        {"ja", "jp"},
        // Korean
        {"ko", "kor"},
    };
    return map.value(isoCode, isoCode);  // pass through if not in map
}

// Strip HTML tags and unescape HTML entities from subtitle text before translation
static QString sanitizeSubtitleText(const QString &raw)
{
    QString text = raw;

    // 1. Strip HTML tags: <b>, <i>, <u>, <font ...>, </b>, etc.
    text.remove(QRegularExpression("<[^>]*>"));

    // 2. Unescape common HTML entities
    text.replace("&amp;",  "&");
    text.replace("&lt;",   "<");
    text.replace("&gt;",   ">");
    text.replace("&quot;", "\"");
    text.replace("&#39;",  "'");
    text.replace("&#039;", "'");
    text.replace("&#34;",  "\"");

    // 3. Strip zero-width / control characters (keep normal spaces, tabs, newlines)
    text.remove(QRegularExpression("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F\\x7F]"));

    // 4. Trim each line and collapse excessive blank lines
    QStringList lines = text.split('\n');
    for (QString &line : lines) {
        line = line.trimmed();
    }
    // Remove trailing empty lines but keep internal spacing
    while (!lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();
    while (!lines.isEmpty() && lines.first().isEmpty())
        lines.removeFirst();

    return lines.join('\n').trimmed();
}

TranslateService::TranslateService(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_requestTimer(new QTimer(this))
    , m_cancelled(false)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &TranslateService::onReplyFinished);
    m_requestTimer->setSingleShot(true);
    connect(m_requestTimer, &QTimer::timeout,
            this, &TranslateService::onRequestTimeout);
}

void TranslateService::startTranslate(const QString &inputSrtPath,
                                       const QString &outputSrtPath,
                                       int engine,
                                       const QString &apiKey,
                                       const QString &apiUrl,
                                       const QString &sourceLang,
                                       const QString &targetLang,
                                       const QString &baiduAppId)
{
    // MUST reset before checking: cancel() sets this =true, and
    // without resetting first, every subsequent startTranslate
    // silently returns without emitting finished() → the caller
    // (VideoSubtitleController) waits forever → stuck.
    m_cancelled = false;
    m_inputSrtPath = inputSrtPath;
    m_outputSrtPath = outputSrtPath;
    m_currentEngine = engine;
    m_currentApiKey = apiKey;
    m_currentApiUrl = apiUrl;
    m_currentSourceLang = sourceLang;
    m_currentTargetLang = targetLang;
    m_baiduAppId = baiduAppId;

    // Parse SRT — preserve timing, only extract subtitle text
    m_entries = SubtitleService::parseSrt(inputSrtPath);
    if (m_entries.isEmpty()) {
        emit finished(false, outputSrtPath, "Failed to parse SRT file or file is empty");
        return;
    }

    m_totalEntries = m_entries.size();
    m_currentEntryIndex = 0;
    m_translatedCount = 0;

    PluginLogger::info(QString("逐句翻译开始: %1 条字幕, 引擎: %2")
        .arg(m_totalEntries).arg(engine == 0 ? "百度翻译" : "未知"));

    // Start translating the first sentence
    translateNext();
}

void TranslateService::cancel()
{
    m_cancelled = true;
    m_requestTimer->stop();
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply = nullptr;
    }
}

void TranslateService::translateNext()
{
    if (m_cancelled) {
        emit finished(false, m_outputSrtPath, "翻译已取消");
        return;
    }

    if (m_currentEntryIndex >= m_totalEntries) {
        // All sentences translated — write output SRT
        writeOutputSrt();
        return;
    }

    const SubtitleService::SubtitleEntry &entry = m_entries.at(m_currentEntryIndex);
    QString textToTranslate = sanitizeSubtitleText(entry.originalText);
    if (textToTranslate.isEmpty()) {
        // Skip empty entries
        m_currentEntryIndex++;
        m_translatedCount++;
        emit progress(static_cast<double>(m_translatedCount) / m_totalEntries);
        translateNext();
        return;
    }

    switch (m_currentEngine) {
    case 0: { // Baidu Translate
        QJsonArray singleText;
        singleText.append(textToTranslate);
        QString fullUrl = buildBaiduUrl(singleText, m_currentTargetLang,
                                         m_currentApiKey, m_baiduAppId);

        PluginLogger::restRequest("GET", fullUrl);

        QNetworkRequest request{QUrl(fullUrl)};
        request.setTransferTimeout(TIMEOUT_MS);  // auto-abort if no data for 30s
        m_currentReply = m_networkManager->get(request);
        m_requestTimer->start(TIMEOUT_MS + 5000);  // safety net: 5s after transfer timeout
        break;
    }
    case 1: { // LibreTranslate (self-hosted, offline NMT)
        QString baseUrl = m_currentApiUrl;
        if (baseUrl.endsWith('/'))
            baseUrl.chop(1);
        QString fullUrl = baseUrl + "/translate";

        QJsonObject body;
        body["q"] = textToTranslate;
        body["source"] = m_currentSourceLang.isEmpty() ? "auto" : m_currentSourceLang;
        body["target"] = m_currentTargetLang;
        body["format"] = "text";
        QByteArray jsonData = QJsonDocument(body).toJson(QJsonDocument::Compact);

        QNetworkRequest request{QUrl(fullUrl)};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setTransferTimeout(TIMEOUT_MS);
        m_currentReply = m_networkManager->post(request, jsonData);
        m_requestTimer->start(TIMEOUT_MS + 5000);

        PluginLogger::restRequest("POST", fullUrl);
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

    // Ignore stale replies from aborted previous retries
    if (reply != m_currentReply) {
        reply->deleteLater();
        return;
    }

    m_requestTimer->stop();
    m_currentReply = nullptr;
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        QString errStr = reply->errorString();
        QString srtEntryInfo = QString("（第 %1 条: \"%2\"）")
            .arg(m_currentEntryIndex + 1)
            .arg(m_entries.at(m_currentEntryIndex).originalText.left(40));
        reply->deleteLater();
        PluginLogger::restResponse(httpStatus, "网络错误: " + errStr + srtEntryInfo);

        m_retryCount++;
        if (m_retryCount <= MAX_RETRIES) {
            PluginLogger::warn(QString("第 %1 条翻译请求失败（第 %2 次重试）: %3")
                .arg(m_currentEntryIndex + 1).arg(m_retryCount).arg(errStr));
            translateNext();
            return;
        }

        emit finished(false, m_outputSrtPath,
                      QString("翻译失败（第 %1 条）: %2").arg(m_currentEntryIndex + 1).arg(errStr));
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    PluginLogger::restResponse(httpStatus, QString::fromUtf8(responseData));
    QJsonObject root = doc.object();

    // Check API error (Baidu: error_code/error_msg; LibreTranslate: error)
    if (root.contains("error_code")) {
        QString errCode = root["error_code"].toString();
        QString errMsg  = root["error_msg"].toString();
        QString srtEntryInfo = QString("（第 %1 条: \"%2\"）")
            .arg(m_currentEntryIndex + 1)
            .arg(m_entries.at(m_currentEntryIndex).originalText.left(40));
        PluginLogger::error(QString("翻译 API 错误 [%1]: %2 %3").arg(errCode, errMsg, srtEntryInfo));
        emit finished(false, m_outputSrtPath,
                      QString("翻译失败（第 %1 条）: [%2] %3")
                          .arg(m_currentEntryIndex + 1).arg(errCode, errMsg));
        return;
    }
    if (root.contains("error")) {
        QString errMsg = root["error"].toString();
        QString srtEntryInfo = QString("（第 %1 条: \"%2\"）")
            .arg(m_currentEntryIndex + 1)
            .arg(m_entries.at(m_currentEntryIndex).originalText.left(40));
        PluginLogger::error(QString("翻译 API 错误: %1 %2").arg(errMsg, srtEntryInfo));
        emit finished(false, m_outputSrtPath,
                      QString("翻译失败（第 %1 条）: %2").arg(m_currentEntryIndex + 1).arg(errMsg));
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
    case 1: { // LibreTranslate
        QString translated = root["translatedText"].toString();
        if (!translated.isEmpty())
            translatedTexts.append(translated);
        break;
    }
    default: {
        emit finished(false, m_outputSrtPath, "Unknown translation engine response");
        return;
    }
    }

    if (translatedTexts.isEmpty()) {
        PluginLogger::error(QString("第 %1 条返回结果为空: \"%2\"")
            .arg(m_currentEntryIndex + 1)
            .arg(m_entries.at(m_currentEntryIndex).originalText.left(40)));
        emit finished(false, m_outputSrtPath,
                      QString("第 %1 条翻译结果为空").arg(m_currentEntryIndex + 1));
        return;
    }

    // Save translated text into the entry
    m_entries[m_currentEntryIndex].translatedText = translatedTexts.first();

    m_currentEntryIndex++;
    m_translatedCount++;
    m_retryCount = 0;  // reset retry counter on success
    emit progress(static_cast<double>(m_translatedCount) / m_totalEntries);

    // Translate next sentence
    translateNext();
}

void TranslateService::onRequestTimeout()
{
    if (m_cancelled) return;

    PluginLogger::warn(QString("第 %1 条翻译请求超时").arg(m_currentEntryIndex + 1));

    // Clean up the hung reply
    if (m_currentReply) {
        m_currentReply->disconnect();
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    m_retryCount++;
    if (m_retryCount <= MAX_RETRIES) {
        PluginLogger::warn(QString("第 %1 条翻译超时重试 (%2/%3)…")
            .arg(m_currentEntryIndex + 1).arg(m_retryCount).arg(MAX_RETRIES));
        translateNext();
        return;
    }

    PluginLogger::error(QString("第 %1 条翻译超时 %2 次，放弃").arg(m_currentEntryIndex + 1).arg(m_retryCount));
    emit finished(false, m_outputSrtPath,
                  QString("翻译超时（第 %1 条）").arg(m_currentEntryIndex + 1));
}

void TranslateService::writeOutputSrt()
{
    bool ok = SubtitleService::writeSrt(m_outputSrtPath, m_entries);
    PluginLogger::info(QString("翻译完成: %1 条字幕 → %2")
        .arg(m_totalEntries).arg(m_outputSrtPath));
    emit finished(ok, m_outputSrtPath, ok ? "" : "Failed to write SRT file");
}

QString TranslateService::buildBaiduUrl(const QJsonArray &texts, const QString &targetLang,
                                         const QString &secretKey, const QString &appId)
{
    // Baidu Translate signing: appid + q + salt + secretKey
    QStringList textList;
    for (const QJsonValue &v : texts) {
        textList.append(v.toString());
    }
    QString query = textList.join("\n");
    QString salt = QString::number(QRandomGenerator::global()->generate());

    QString signStr = appId + query + salt + secretKey;
    QString sign = QCryptographicHash::hash(signStr.toUtf8(), QCryptographicHash::Md5).toHex().toLower();

    // URL-encode q (sign uses raw q)
    QString encodedQ = QString::fromUtf8(QUrl::toPercentEncoding(query));
    // Use detected source language (from SRT content analysis) instead of "auto"
    // so Baidu API gets a reliable `from` hint (e.g., jp → zh, en → zh)
    // Map ISO 639-1 codes to Baidu-specific codes (e.g. ja→jp, ko→kor)
    QString fromLang = m_currentSourceLang.isEmpty() ? "auto" : toBaiduLangCode(m_currentSourceLang);
    QString toLang = toBaiduLangCode(targetLang);
    QString url = QString("http://api.fanyi.baidu.com/api/trans/vip/translate"
                          "?q=%1&from=%2&to=%3&appid=%4&salt=%5&sign=%6")
                      .arg(encodedQ, fromLang, toLang, appId, salt, sign);
    return url;
}
