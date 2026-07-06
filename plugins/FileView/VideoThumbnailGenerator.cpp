#include "VideoThumbnailGenerator.h"
#include "GlobalConfig.h"
#include "FileViewPlugin.h"
#include "FfmpegUtils.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QCryptographicHash>
#include <QDebug>

VideoThumbnailGenerator::VideoThumbnailGenerator(QObject *parent)
    : QObject(parent)
{
}

VideoThumbnailGenerator::~VideoThumbnailGenerator()
{
    cancel();
}

void VideoThumbnailGenerator::setFfmpegPath(const QString &path)
{
    m_ffmpegPath = path;
}

void VideoThumbnailGenerator::requestThumbnails(const QList<ThumbnailRequest> &requests)
{
    if (m_ffmpegPath.isEmpty())
        return;

    for (const auto &req : requests) {
        QString cachePath = cacheFilePath(req.filePath, req.modifiedTime);
        if (QFileInfo::exists(cachePath))
            emit thumbnailReady(req.filePath, cachePath);
        else
            m_queue.enqueue(req);
    }

    if (!m_processing && !m_queue.isEmpty())
        processNext();
}

void VideoThumbnailGenerator::cancel()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(500);
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_queue.clear();
    m_processing = false;
}

void VideoThumbnailGenerator::processNext()
{
    if (m_queue.isEmpty()) {
        m_processing = false;
        return;
    }

    m_processing = true;
    ThumbnailRequest req = m_queue.dequeue();
    QString outPath = cacheFilePath(req.filePath, req.modifiedTime);

    QDir().mkpath(QFileInfo(outPath).absolutePath());

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    qint64 durationMs = getVideoDuration(m_ffmpegPath, req.filePath);
    double durationSec = durationMs / 1000.0;
    int maxSeek = 0;
    if (durationSec > 0) {
        bool isLongVideo = (durationSec / 10.0 >= 30);
        if (req.retryCount == 0) {
            if (isLongVideo) {
                req.seekTime = 5;
                maxSeek = 30;
            } else {
                req.seekTime = static_cast<int>(durationSec / 10.0);
                maxSeek = static_cast<int>(durationSec) - 2;
            }
        } else {
            maxSeek = isLongVideo ? 30 : static_cast<int>(durationSec) - 2;
        }
    }

    m_process = new QProcess(this);
    QProcess *proc = m_process;
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, req, outPath, maxSeek](int exitCode, QProcess::ExitStatus status) {
        if (m_process != proc)
            return;

        m_process->deleteLater();
        m_process = nullptr;

        if (status == QProcess::NormalExit && exitCode == 0
            && QFileInfo::exists(outPath)) {
            int nextSeek = req.seekTime + 5;
            bool canRetry = req.retryCount < 5 && nextSeek <= maxSeek;
            if (canRetry && !isKeyframeValid(outPath)) {
                QFile::remove(outPath);
                ThumbnailRequest retry = req;
                retry.seekTime = nextSeek;
                retry.retryCount = req.retryCount + 1;
                m_queue.prepend(retry);
            } else {
                emit thumbnailReady(req.filePath, outPath);
            }
        }

        processNext();
    });

    m_process->start(m_ffmpegPath, {
        "-ss", QString::number(req.seekTime),
        "-i", QDir::toNativeSeparators(req.filePath),
        "-vframes", "1",
        "-q:v", "2",
        "-y",
        QDir::toNativeSeparators(outPath)
    });
}

bool VideoThumbnailGenerator::isKeyframeValid(const QString &imagePath)
{
    QImage img(imagePath);
    if (img.isNull())
        return false;

    img = img.scaled(64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    double sum = 0.0, sumSq = 0.0;
    int count = 0;

    for (int y = 0; y < img.height(); y += 2) {
        for (int x = 0; x < img.width(); x += 2) {
            QRgb pixel = img.pixel(x, y);
            double luma = 0.299 * qRed(pixel)
                        + 0.587 * qGreen(pixel)
                        + 0.114 * qBlue(pixel);
            sum += luma;
            sumSq += luma * luma;
            ++count;
        }
    }

    double mean = sum / count;
    double variance = sumSq / count - mean * mean;
    double stddev = qSqrt(qMax(0.0, variance));

    return stddev >= 30.0;
}

QString VideoThumbnailGenerator::cacheFilePath(
    const QString &filePath, const QDateTime &modifiedTime) const
{
    QString dir = GlobalConfig::cachePath() + "/" + FileViewPlugin::PluginKey + "/thumbnails";
    QDir().mkpath(dir);
    QByteArray data = (filePath + modifiedTime.toUTC().toString(Qt::ISODate)).toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
    return dir + QStringLiteral("/") + QString::fromLatin1(hash) + QStringLiteral(".jpg");
}
