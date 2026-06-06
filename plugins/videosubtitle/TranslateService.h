#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>

class TranslateService : public QObject
{
    Q_OBJECT
public:
    explicit TranslateService(QObject *parent = nullptr);

    void startTranslate(const QString &inputSrtPath,
                        const QString &outputSrtPath,
                        int engine,
                        const QString &apiKey,
                        const QString &apiUrl,
                        const QString &targetLang,
                        const QString &baiduAppId = QString(),
                        bool bilingual = true);

    void cancel();

signals:
    void progress(double value);
    void finished(bool success, const QString &outputPath, const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    void translateBatch(const QJsonArray &texts, int engine,
                        const QString &apiKey, const QString &apiUrl,
                        const QString &targetLang, int batchIndex, int totalBatches,
                        const QString &baiduAppId = QString());

    QString buildBaiduUrl(const QJsonArray &texts, const QString &targetLang,
                          const QString &secretKey, const QString &appId);

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;

    // Translation state
    QList<int> m_pendingBatchIndices;
    QJsonArray m_translatedTexts;
    int m_totalBatches = 0;
    int m_completedBatches = 0;
    bool m_cancelled = false;

    // Current batch info
    int m_currentBatchIndex = 0;
    QString m_inputSrtPath;
    QString m_outputSrtPath;
    int m_currentEngine = 0;
    QString m_currentApiKey;
    QString m_currentApiUrl;
    QString m_currentTargetLang;
    bool m_bilingual = true;
};
