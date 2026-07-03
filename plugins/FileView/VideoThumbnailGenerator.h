#pragma once

#include <QObject>
#include <QString>
#include <QQueue>
#include <QDateTime>
#include <QProcess>

class VideoThumbnailGenerator : public QObject
{
    Q_OBJECT

public:
    struct ThumbnailRequest {
        QString filePath;
        QDateTime modifiedTime;
        int seekTime = 30;
    };

    explicit VideoThumbnailGenerator(QObject *parent = nullptr);
    ~VideoThumbnailGenerator() override;

    void setFfmpegPath(const QString &path);
    void requestThumbnails(const QList<ThumbnailRequest> &requests);
    void cancel();

signals:
    void thumbnailReady(const QString &filePath, const QString &thumbnailPath);

private:
    void processNext();
    static bool isMostlyBlack(const QString &imagePath);
    QString cacheFilePath(const QString &filePath, const QDateTime &modifiedTime) const;

    QString m_ffmpegPath;
    QQueue<ThumbnailRequest> m_queue;
    QProcess *m_process = nullptr;
    bool m_processing = false;
};
