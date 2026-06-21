#include "ImageConverterController.h"
#include "ImageConverterPlugin.h"
#include "ImageConverterSettings.h"
#include "Config.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QUrl>

ImageConverterController::ImageConverterController(PluginLogger *logger, ImageConverterSettings *settings, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(settings)
{
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
    QString rootPath = m_settings->rootPath();
    int targetFormat = m_settings->targetFormat();
    int quality = m_settings->quality();
    QString bgColor = m_settings->bgColor();
    int outputMode = m_settings->outputMode();
    QString outputDir = m_settings->outputDir();
    bool recursive = m_settings->recursive();

    if (rootPath.isEmpty() || !QDir(rootPath).exists()) {
        setStatusMessage(QStringLiteral("请选择有效的源文件夹"));
        m_logger->warn("转换失败: 源文件夹无效");
        return;
    }

    m_logger->info(QString("===== 开始图片格式转换 ====="));
    m_logger->info(QString("源目录: %1, 目标格式: %2, 递归=%3")
        .arg(rootPath).arg(formatExtension(targetFormat)).arg(recursive));

    m_records.clear();
    emit recordsChanged();
    emit hasRecordsChanged();

    setIsProcessing(true);

    int successCount = 0;
    int failCount = 0;
    int skipCount = 0;

    processDirectory(QDir(rootPath), QString(), successCount, failCount, skipCount,
                     rootPath, targetFormat, outputMode, outputDir, quality, bgColor, recursive);

    QStringList parts;
    if (successCount > 0) parts << QStringLiteral("成功 %1 个").arg(successCount);
    if (failCount > 0) parts << QStringLiteral("失败 %1 个").arg(failCount);
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
                                                 int &successCount, int &failCount, int &skipCount,
                                                 const QString &rootPath, int targetFormat,
                                                 int outputMode, const QString &outputDir,
                                                 int quality, const QString &bgColor, bool recursive)
{
    m_logger->info(QString("处理目录: %1").arg(currentDir.absolutePath()));
    int dirSuccess = 0, dirFail = 0, dirSkip = 0;

    QFileInfoList entries = currentDir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    QString targetExt = formatExtension(targetFormat);
    QByteArray targetFmt = formatForQImage(targetFormat);

    for (const QFileInfo &entry : entries) {
        if (!isImageFile(entry.fileName())) continue;

        QString srcExt = entry.suffix().toLower();
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

        if (outputMode == 0) {
            destPath = currentDir.absoluteFilePath(newBaseName);
        } else {
            QString outRoot = outputDir.isEmpty()
                ? rootPath + "_converted"
                : outputDir;
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

        if (targetFormat == 1 && image.hasAlphaChannel()) {
            QImage filled(image.size(), QImage::Format_RGB32);
            filled.fill(bgColor);
            QPainter painter(&filled);
            painter.drawImage(0, 0, image);
            painter.end();
            image = filled;
        }

        bool ok = image.save(destPath, targetFmt, quality);
        if (ok) {
            if (outputMode == 0) {
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

    if (recursive) {
        QFileInfoList dirs = currentDir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &dirEntry : dirs) {
            QString subRelative = relativePath.isEmpty()
                ? dirEntry.fileName()
                : relativePath + "/" + dirEntry.fileName();
            processDirectory(QDir(dirEntry.absoluteFilePath()), subRelative,
                             successCount, failCount, skipCount,
                             rootPath, targetFormat, outputMode, outputDir,
                             quality, bgColor, recursive);
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

void ImageConverterController::restoreRecord(int index)
{
    if (index < 0 || index >= m_records.size())
        return;

    ConvertRecord &record = m_records[index];
    if (!record.success)
        return;

    if (!QFileInfo::exists(record.newPath)) {
        record.status = QStringLiteral("失败：文件不存在");
        record.success = false;
        setStatusMessage(QStringLiteral("还原失败：文件不存在"));
        m_logger->warn(QString("还原失败: %1 — 文件不存在").arg(record.newPath));
        emit recordsChanged();
        return;
    }

    if (QFileInfo::exists(record.originalPath)) {
        // Original still exists (outputMode 1): remove the converted copy
        if (QFile::remove(record.newPath)) {
            record.status = QStringLiteral("已还原");
            record.success = false;
            setStatusMessage(QStringLiteral("已还原：%1").arg(record.originalName));
            m_logger->info(QString("已还原: %1").arg(record.originalName));
        } else {
            record.status = QStringLiteral("失败：删除失败");
            record.success = false;
            setStatusMessage(QStringLiteral("还原失败：%1").arg(record.newName));
            m_logger->error(QString("还原失败: %1 — 删除失败").arg(record.newName));
        }
    } else {
        // Original was deleted (outputMode 0): rename converted file back to original name
        QDir parentDir = QFileInfo(record.newPath).absoluteDir();
        if (parentDir.rename(QFileInfo(record.newPath).fileName(),
                             QFileInfo(record.originalPath).fileName())) {
            record.status = QStringLiteral("已还原");
            record.success = false;
            setStatusMessage(QStringLiteral("已还原：%1").arg(record.originalName));
            m_logger->info(QString("已还原: %1 → %2").arg(record.newName, record.originalName));
        } else {
            record.status = QStringLiteral("失败：还原失败");
            record.success = false;
            setStatusMessage(QStringLiteral("还原失败：%1").arg(record.newName));
            m_logger->error(QString("还原失败: %1").arg(record.newName));
        }
    }

    emit recordsChanged();
}

void ImageConverterController::restoreAllRecords()
{
    m_logger->info("===== 开始批量还原 =====");
    int successCount = 0;
    int failCount = 0;

    for (int i = 0; i < m_records.size(); ++i) {
        ConvertRecord &record = m_records[i];
        if (!record.success)
            continue;

        if (!QFileInfo::exists(record.newPath)) {
            record.status = QStringLiteral("失败：文件不存在");
            record.success = false;
            failCount++;
            continue;
        }

        bool ok = false;
        if (QFileInfo::exists(record.originalPath)) {
            // Original exists → delete converted copy
            ok = QFile::remove(record.newPath);
        } else {
            // Original gone → rename converted file back
            QDir parentDir = QFileInfo(record.newPath).absoluteDir();
            ok = parentDir.rename(QFileInfo(record.newPath).fileName(),
                                  QFileInfo(record.originalPath).fileName());
        }

        if (ok) {
            record.status = QStringLiteral("已还原");
            record.success = false;
            successCount++;
        } else {
            record.status = QStringLiteral("失败：还原失败");
            record.success = false;
            failCount++;
        }
    }

    if (successCount == 0 && failCount == 0) {
        setStatusMessage(QStringLiteral("没有可还原的记录"));
        m_logger->info("批量还原完成: 没有可还原的记录");
    } else if (failCount == 0) {
        setStatusMessage(QStringLiteral("已全部还原"));
        m_logger->info(QString("批量还原完成: 成功还原 %1 个").arg(successCount));
    } else {
        setStatusMessage(QStringLiteral("成功还原 %1 个，失败 %2 个").arg(successCount).arg(failCount));
        m_logger->info(QString("批量还原完成: 成功 %1 个，失败 %2 个").arg(successCount).arg(failCount));
    }

    emit recordsChanged();
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
    cancel();
    m_records.clear();
    setStatusMessage(QString());

    emit recordsChanged();
    emit hasRecordsChanged();
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
