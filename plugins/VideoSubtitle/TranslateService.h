#pragma once

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include "SubtitleService.h"

class PluginLogger;

class TranslateService : public QObject
{
    Q_OBJECT
public:
    explicit TranslateService(PluginLogger *logger, QObject *parent = nullptr);

    void startTranslate(const QString &inputSrtPath,
                        const QString &outputSrtPath,
                        int engine,
                        const QString &apiKey,
                        const QString &apiUrl,
                        const QString &sourceLang,
                        const QString &targetLang,
                        const QString &baiduAppId = QString());

    void cancel();

signals:
    void progress(double value);
    void finished(bool success, const QString &outputPath, const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);
    void onRequestTimeout();

private:
    void translateNext();
    void writeOutputSrt();

    QString buildBaiduUrl(const QJsonArray &texts, const QString &targetLang,
                          const QString &secretKey, const QString &appId);

    PluginLogger *m_logger = nullptr;
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;

    // Per-request timeout (30s) with retry
    QTimer *m_requestTimer;
    int m_retryCount = 0;
    static constexpr int MAX_RETRIES = 2;
    static constexpr int TIMEOUT_MS = 30000;

    // Translation state — per-sentence
    QList<SubtitleService::SubtitleEntry> m_entries;
    int m_currentEntryIndex = 0;
    int m_totalEntries = 0;
    int m_translatedCount = 0;
    bool m_cancelled = false;

    QString m_inputSrtPath;
    QString m_outputSrtPath;
    int m_currentEngine = 0;
    QString m_currentApiKey;
    QString m_currentApiUrl;
    QString m_currentSourceLang;   // detected from SRT content
    QString m_currentTargetLang;
    QString m_baiduAppId;
};
