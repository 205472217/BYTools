#include "VideoThumbnailGenerator.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>
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

    m_process = new QProcess(this);
    QProcess *proc = m_process;
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, req, outPath](int exitCode, QProcess::ExitStatus status) {
        if (m_process != proc)
            return;

        m_process->deleteLater();
        m_process = nullptr;

        if (status == QProcess::NormalExit && exitCode == 0
            && QFileInfo::exists(outPath)) {
            if (req.seekTime < 60 && isMostlyBlack(outPath)) {
                QFile::remove(outPath);
                ThumbnailRequest retry = req;
                retry.seekTime += 5;
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

bool VideoThumbnailGenerator::isMostlyBlack(const QString &imagePath)
{
    QImage img(imagePath);
    if (img.isNull())
        return false;

    img = img.scaled(64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    int blackCount = 0;
    int total = img.width() * img.height();

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QRgb pixel = img.pixel(x, y);
            if (qRed(pixel) < 30 && qGreen(pixel) < 30 && qBlue(pixel) < 30)
                ++blackCount;
        }
    }

    return blackCount > total / 2;
}

QString VideoThumbnailGenerator::cacheDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/fileview/thumbnails");
}

QString VideoThumbnailGenerator::cacheFilePath(
    const QString &filePath, const QDateTime &modifiedTime) const
{
    QByteArray data = (filePath + modifiedTime.toUTC().toString(Qt::ISODate)).toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
    return cacheDir() + QStringLiteral("/") + QString::fromLatin1(hash) + QStringLiteral(".jpg");
}
