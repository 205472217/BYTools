#include "ImageConverterController.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QUrl>

ImageConverterController::ImageConverterController(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
{
}

QString ImageConverterController::rootPath() const { return m_rootPath; }

void ImageConverterController::setRootPath(const QString &rootPath)
{
    QString normalizedPath = rootPath;
    const QUrl url(rootPath);
    if (url.isLocalFile()) {
        normalizedPath = url.toLocalFile();
    }
    if (m_rootPath == normalizedPath) return;
    m_rootPath = normalizedPath;
    emit rootPathChanged();
}

int ImageConverterController::targetFormat() const { return m_targetFormat; }
void ImageConverterController::setTargetFormat(int format)
{
    if (m_targetFormat == format) return;
    m_targetFormat = format;
    emit targetFormatChanged();
}

int ImageConverterController::quality() const { return m_quality; }
void ImageConverterController::setQuality(int quality)
{
    if (m_quality == quality) return;
    m_quality = quality;
    emit qualityChanged();
}

QString ImageConverterController::bgColor() const { return m_bgColor; }
void ImageConverterController::setBgColor(const QString &color)
{
    if (m_bgColor == color) return;
    m_bgColor = color;
    emit bgColorChanged();
}

int ImageConverterController::outputMode() const { return m_outputMode; }
void ImageConverterController::setOutputMode(int mode)
{
    if (m_outputMode == mode) return;
    m_outputMode = mode;
    emit outputModeChanged();
}

QString ImageConverterController::outputDir() const { return m_outputDir; }
void ImageConverterController::setOutputDir(const QString &dir)
{
    if (m_outputDir == dir) return;
    m_outputDir = dir;
    emit outputDirChanged();
}

bool ImageConverterController::recursive() const { return m_recursive; }
void ImageConverterController::setRecursive(bool recursive)
{
    if (m_recursive == recursive) return;
    m_recursive = recursive;
    emit recursiveChanged();
}

QString ImageConverterController::statusMessage() const { return m_statusMessage; }
bool ImageConverterController::hasRecords() const { return !m_records.isEmpty(); }

QVariantList ImageConverterController::records() const
{
    QVariantList result;
    for (const auto &record : m_records) {
        QVariantMap map;
        map["originalPath"] = record.originalPath;
        map["newPath"] = record.newPath;
        map["originalName"] = record.originalName;
        map["newName"] = record.newName;
        map["formatTag"] = record.formatTag;
        map["success"] = record.success;
        map["status"] = record.status;
        result.append(map);
    }
    return result;
}

void ImageConverterController::executeConvert()
{
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        setStatusMessage(QStringLiteral("请选择有效的源文件夹"));
        m_logger->warn("转换失败: 源文件夹无效");
        return;
    }

    m_logger->info(QString("===== 开始图片格式转换 ====="));
    m_logger->info(QString("源目录: %1, 目标格式: %2, 递归=%3")
        .arg(m_rootPath).arg(formatExtension(m_targetFormat)).arg(m_recursive));

    m_records.clear();
    emit recordsChanged();
    emit hasRecordsChanged();

    setIsProcessing(true);

    int successCount = 0;
    int failCount = 0;
    int skipCount = 0;

    processDirectory(QDir(m_rootPath), QString(), successCount, failCount, skipCount);

    QStringList parts;
    if (successCount > 0) parts << QStringLiteral("成功 %1 个").arg(successCount);
    if (failCount > 0) parts << QStringLiteral("失败 %2 个").arg(failCount);
    if (skipCount > 0) parts << QStringLiteral("跳过 %1 个").arg(skipCount);

    if (parts.isEmpty()) {
        setStatusMessage(QStringLiteral("没有找到图片文件"));
        m_logger->info("转换完成: 没有找到图片文件");
    } else {
        setStatusMessage(parts.join("，"));
        m_logger->info(QString("转换完成: %1").arg(parts.join(", ")));
    }

    emit recordsChanged();
    emit hasRecordsChanged();

    setIsProcessing(false);
}

void ImageConverterController::processDirectory(const QDir &currentDir, const QString &relativePath,
                                                 int &successCount, int &failCount, int &skipCount)
{
    m_logger->info(QString("处理目录: %1").arg(currentDir.absolutePath()));
    int dirSuccess = 0, dirFail = 0, dirSkip = 0;

    QFileInfoList entries = currentDir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    QString targetExt = formatExtension(m_targetFormat);
    QByteArray targetFmt = formatForQImage(m_targetFormat);

    for (const QFileInfo &entry : entries) {
        if (!isImageFile(entry.fileName())) continue;

        QString srcExt = entry.suffix().toLower();
        // 同格式自动跳过
        if (QStringLiteral(".") + srcExt == targetExt) {
            addRecord(entry.absoluteFilePath(), entry.absoluteFilePath(),
                      formatTagForExt(srcExt), true, QStringLiteral("已跳过"));
            skipCount++;
            dirSkip++;
            m_logger->info(QString("  [跳过] %1 — 已是目标格式").arg(entry.fileName()));
            continue;
        }

        QString newBaseName = entry.completeBaseName() + targetExt;
        QString destPath;

        if (m_outputMode == 0) {
            // 替换原文件
            destPath = currentDir.absoluteFilePath(newBaseName);
        } else {
            // 输出到新目录
            QString outRoot = m_outputDir.isEmpty()
                ? m_rootPath + "_converted"
                : m_outputDir;
            QString destDir = relativePath.isEmpty() ? outRoot : outRoot + "/" + relativePath;
            QDir().mkpath(destDir);
            destPath = destDir + "/" + newBaseName;
        }

        QImage image;
        if (entry.size() > 0) {
            image = QImage(entry.absoluteFilePath());
        }
        if (image.isNull()) {
            addRecord(entry.absoluteFilePath(), destPath,
                      formatTagForExt(srcExt), false, QStringLiteral("失败：无法读取"));
            dirFail++;
            failCount++;
            continue;
        }

        // JPG 不支持 alpha，需要填充背景色
        if (m_targetFormat == 1 && image.hasAlphaChannel()) {
            QImage filled(image.size(), QImage::Format_RGB32);
            filled.fill(m_bgColor);
            QPainter painter(&filled);
            painter.drawImage(0, 0, image);
            painter.end();
            image = filled;
        }

        bool ok = image.save(destPath, targetFmt, m_quality);
        if (ok) {
            // 替换模式下，转换成功后删除原文件
            if (m_outputMode == 0) {
                QFile::remove(entry.absoluteFilePath());
            }
            addRecord(entry.absoluteFilePath(), destPath,
                      formatTagForExt(srcExt), true, QStringLiteral("已转换"));
            successCount++;
            dirSuccess++;
            m_logger->info(QString("  [转换] %1 → %2").arg(entry.fileName(), QFileInfo(destPath).fileName()));
        } else {
            addRecord(entry.absoluteFilePath(), destPath,
                      formatTagForExt(srcExt), false, QStringLiteral("失败：保存失败"));
            dirFail++;
            failCount++;
            m_logger->error(QString("  [失败] %1 — 保存失败").arg(entry.fileName()));
        }
    }

    m_logger->info(QString("目录处理完成: %1 (成功=%2, 失败=%3, 跳过=%4)")
        .arg(currentDir.absolutePath()).arg(dirSuccess).arg(dirFail).arg(dirSkip));

    if (m_recursive) {
        QFileInfoList dirs = currentDir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &dirEntry : dirs) {
            QString subRelative = relativePath.isEmpty()
                ? dirEntry.fileName()
                : relativePath + "/" + dirEntry.fileName();
            processDirectory(QDir(dirEntry.absoluteFilePath()), subRelative,
                             successCount, failCount, skipCount);
        }
    }
}

void ImageConverterController::clearRecords()
{
    m_records.clear();
    emit recordsChanged();
    emit hasRecordsChanged();
    setStatusMessage(QString());
}

bool ImageConverterController::isProcessing() const
{
    return m_isProcessing;
}

void ImageConverterController::cancel()
{
    if (m_isProcessing) {
        m_logger->info("图片转换已取消");
        setIsProcessing(false);
        setStatusMessage("已取消");
    }
}

void ImageConverterController::setIsProcessing(bool processing)
{
    if (m_isProcessing == processing) return;
    m_isProcessing = processing;
    emit isProcessingChanged();
}

void ImageConverterController::reset()
{
    m_records.clear();
    m_rootPath.clear();
    m_targetFormat = 1;
    m_quality = 85;
    m_bgColor = "#ffffff";
    m_outputMode = 0;
    m_outputDir.clear();
    m_recursive = false;
    setStatusMessage(QString());

    emit recordsChanged();
    emit hasRecordsChanged();
    emit rootPathChanged();
    emit targetFormatChanged();
    emit qualityChanged();
    emit bgColorChanged();
    emit outputModeChanged();
    emit outputDirChanged();
    emit recursiveChanged();
}

void ImageConverterController::addRecord(const QString &originalPath, const QString &newPath,
                                          const QString &formatTag, bool success, const QString &status)
{
    ConvertRecord record;
    record.originalPath = originalPath;
    record.newPath = newPath;
    record.originalName = QFileInfo(originalPath).fileName();
    record.newName = QFileInfo(newPath).fileName();
    record.formatTag = formatTag;
    record.success = success;
    record.status = status;
    m_records.append(record);
}

void ImageConverterController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) return;
    m_statusMessage = message;
    emit statusMessageChanged();
}

bool ImageConverterController::isImageFile(const QString &fileName) const
{
    QString ext = QString(".") + QFileInfo(fileName).suffix().toLower();
    // 排除目标格式本身由大小写不同导致的误判，直接比较小写
    return m_imageExtensions.contains(ext);
}

QString ImageConverterController::formatExtension(int formatIndex)
{
    static const QStringList exts = {".png", ".jpg", ".bmp", ".webp", ".tiff"};
    return (formatIndex >= 0 && formatIndex < exts.size()) ? exts[formatIndex] : ".png";
}

QByteArray ImageConverterController::formatForQImage(int formatIndex)
{
    static const QList<QByteArray> fmts = {"PNG", "JPEG", "BMP", "WEBP", "TIFF"};
    return (formatIndex >= 0 && formatIndex < fmts.size()) ? fmts[formatIndex] : "PNG";
}

QString ImageConverterController::formatTagForExt(const QString &ext)
{
    QString e = ext.toLower();
    if (e == "png") return "PNG";
    if (e == "jpg" || e == "jpeg") return "JPG";
    if (e == "bmp") return "BMP";
    if (e == "webp") return "WEBP";
    if (e == "tiff" || e == "tif") return "TIFF";
    if (e == "gif") return "GIF";
    if (e == "ico") return "ICO";
    return e.toUpper();
}
