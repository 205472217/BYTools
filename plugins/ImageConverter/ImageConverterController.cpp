#include "ImageConverterController.h"
#include "ImageConverterPlugin.h"
#include "ImageConverterSettings.h"
#include "Config.h"
#include "Logger.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QUrl>

ImageConverterController::ImageConverterController(PluginLogger *logger, ImageConverterSettings *settings, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(settings)
{
    connect(&m_workerThread, &QThread::started,
            this, &ImageConverterController::doWork, Qt::DirectConnection);
    connect(&m_workerThread, &QThread::finished, this, [this]() {
        m_workerRunning = false;
    });
}

ImageConverterController::~ImageConverterController()
{
    cancel();
    m_workerThread.quit();
    m_workerThread.wait(5000);
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
    bool isSingle = (m_settings->mode() == 0);

    if (isSingle) {
        if (rootPath.isEmpty() || !QFileInfo::exists(rootPath)) {
            setStatusMessage(QStringLiteral("请选择有效的图片文件"));
            m_logger->warn("转换失败: 文件无效");
            return;
        }
        if (!isImageFile(QFileInfo(rootPath).fileName())) {
            setStatusMessage(QStringLiteral("请选择图片文件"));
            m_logger->warn("转换失败: 不支持的文件格式");
            return;
        }
    } else {
        if (rootPath.isEmpty() || !QDir(rootPath).exists()) {
            setStatusMessage(QStringLiteral("请选择有效的源文件夹"));
            m_logger->warn("转换失败: 源文件夹无效");
            return;
        }
    }

    bool doConvert = m_settings->convertEnabled();
    bool doResize = m_settings->resizeEnabled();

    if (!doConvert && !doResize) {
        setStatusMessage(QStringLiteral("请至少勾选「格式转换」或「宽高缩放」"));
        m_logger->warn("转换失败: 未勾选任何处理选项");
        return;
    }

    int targetFormat = m_settings->targetFormat();
    bool recursive = m_settings->recursive();
    m_logger->info(QString("===== 开始图片处理 ====="));
    m_logger->info(QString("源路径: %1, 单文件=%2, 格式转换=%3, 宽高缩放=%4, 递归=%5")
        .arg(rootPath).arg(isSingle).arg(doConvert).arg(doResize).arg(recursive));
    emit logMessage(QString("源路径: %1").arg(rootPath));
    if (recursive)
        emit logMessage("  启用了递归查找，正在遍历子目录...");

    m_records.clear();
    emit recordsChanged();
    emit hasRecordsChanged();

    setIsProcessing(true);
    m_workerRunning = true;
    m_workerThread.start();
}

void ImageConverterController::processSingleFile(
    const QString &filePath, const QString &destPath,
    bool doConvert, int targetFormat, int quality,
    bool doResize, int resizeMode, double resizeRatio,
    int resizeWidth, int resizeHeight, const QString &bgColor,
    const QString &targetExt, const QByteArray &targetFmt,
    int outputMode, int &successCount, int &failCount, int &skipCount,
    QList<ConvertRecord> &records)
{
    QFileInfo entry(filePath);
    QString srcExt = entry.suffix().toLower();
    QString srcExtDot = QStringLiteral(".") + srcExt;

    if (doConvert && srcExtDot == targetExt && !doResize) {
        if (outputMode == 1) {
            if (QFile::copy(entry.absoluteFilePath(), destPath)) {
                records.append({
                    entry.absoluteFilePath(), destPath,
                    entry.fileName(), QFileInfo(destPath).fileName(),
                    formatTagForExt(srcExt), true, QStringLiteral("已复制")
                });
                successCount++;
                emit logMessage(QString("  [复制] %1 → %2").arg(entry.fileName(), QFileInfo(destPath).fileName()));
            } else if (QFileInfo::exists(destPath)) {
                records.append({
                    entry.absoluteFilePath(), destPath,
                    entry.fileName(), QFileInfo(destPath).fileName(),
                    formatTagForExt(srcExt), true, QStringLiteral("已存在")
                });
                successCount++;
                emit logMessage(QString("  [跳过] %1 — 输出文件已存在").arg(entry.fileName()));
            } else {
                records.append({
                    entry.absoluteFilePath(), destPath,
                    entry.fileName(), QFileInfo(destPath).fileName(),
                    formatTagForExt(srcExt), false, QStringLiteral("失败：复制失败")
                });
                failCount++;
                emit logMessage(QString("  [失败] %1 — 复制失败").arg(entry.fileName()));
            }
        } else {
            records.append({
                entry.absoluteFilePath(), entry.absoluteFilePath(),
                entry.fileName(), entry.fileName(),
                formatTagForExt(srcExt), true, QStringLiteral("已跳过")
            });
            skipCount++;
        }
        return;
    }

    QImage image;
    if (entry.size() > 0)
        image = QImage(entry.absoluteFilePath());

    if (image.isNull()) {
        records.append({
            entry.absoluteFilePath(), destPath,
            entry.fileName(), QFileInfo(destPath).fileName(),
            formatTagForExt(srcExt), false, QStringLiteral("失败：无法读取")
        });
        failCount++;
        return;
    }

    // 宽高缩放
    if (doResize) {
        if (resizeMode == 0) {
            int newW = qRound(image.width() * resizeRatio);
            int newH = qRound(image.height() * resizeRatio);
            image = image.scaled(newW, newH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            image = image.scaled(resizeWidth, resizeHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }

    // JPG背景色填充
    if (doConvert && targetFormat == 1 && image.hasAlphaChannel()) {
        QImage filled(image.size(), QImage::Format_RGB32);
        filled.fill(bgColor);
        QPainter painter(&filled);
        painter.drawImage(0, 0, image);
        painter.end();
        image = filled;
    }

    if (image.save(destPath, doConvert ? targetFmt.constData() : nullptr, doConvert ? quality : -1)) {
        records.append({
            entry.absoluteFilePath(), destPath,
            entry.fileName(), QFileInfo(destPath).fileName(),
            formatTagForExt(srcExt), true, QStringLiteral("已处理")
        });
        successCount++;
        emit logMessage(QString("  [处理] %1 → %2").arg(entry.fileName(), QFileInfo(destPath).fileName()));
    } else {
        records.append({
            entry.absoluteFilePath(), destPath,
            entry.fileName(), QFileInfo(destPath).fileName(),
            formatTagForExt(srcExt), false, QStringLiteral("失败：保存失败")
        });
        failCount++;
        emit logMessage(QString("  [失败] %1 — 保存失败").arg(entry.fileName()));
    }
}

void ImageConverterController::doWork()
{
    QString rootPath = m_settings->rootPath();
    int targetFormat = m_settings->targetFormat();
    int quality = m_settings->quality();
    QString bgColor = m_settings->bgColor();
    int outputMode = m_settings->outputMode();
    QString outputDir = m_settings->outputDir();
    bool recursive = m_settings->recursive();
    bool doConvert = m_settings->convertEnabled();
    bool doResize = m_settings->resizeEnabled();
    int resizeMode = m_settings->resizeMode();
    double resizeRatio = m_settings->resizeRatio();
    int resizeWidth = m_settings->resizeWidth();
    int resizeHeight = m_settings->resizeHeight();
    bool isSingle = (m_settings->mode() == 0);

    int successCount = 0, failCount = 0, skipCount = 0;
    QList<ConvertRecord> records;

    QString targetExt;
    QByteArray targetFmt;
    if (doConvert) {
        targetExt = formatExtension(targetFormat);
        targetFmt = formatForQImage(targetFormat);
    }

    auto procFile = [&](const QString &filePath, const QString &destDir, const QString &relPath) {
        QFileInfo entry(filePath);
        if (!isImageFile(entry.fileName())) return;

        QString destFileName;
        if (doConvert) {
            destFileName = entry.completeBaseName() + targetExt;
        } else {
            destFileName = entry.fileName();
        }
        QString destPath;

        if (outputMode == 0) {
            destPath = entry.dir().absoluteFilePath(destFileName);
        } else {
            QString outRoot = outputDir.isEmpty() ? rootPath + "_converted" : outputDir;
            QString destDir2 = relPath.isEmpty() ? outRoot : outRoot + "/" + relPath;
            QDir().mkpath(destDir2);
            destPath = destDir2 + "/" + destFileName;
        }

        processSingleFile(filePath, destPath,
            doConvert, targetFormat, quality,
            doResize, resizeMode, resizeRatio,
            resizeWidth, resizeHeight, bgColor,
            targetExt, targetFmt, outputMode,
            successCount, failCount, skipCount, records);
    };

    if (isSingle) {
        QFileInfo fi(rootPath);
        QString destDir = fi.dir().absolutePath();
        procFile(rootPath, destDir, QString());
    } else {
        std::function<void(const QString &, const QString &)> procDir;
        procDir = [&](const QString &dirPath, const QString &relPath) {
            QDir currentDir(dirPath);
            QFileInfoList entries = currentDir.entryInfoList(
                QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

            for (const QFileInfo &entry : entries) {
                procFile(entry.absoluteFilePath(), QString(), relPath);
            }

            if (recursive) {
                QFileInfoList dirs = currentDir.entryInfoList(
                    QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
                for (const QFileInfo &dirEntry : dirs) {
                    QString subRel = relPath.isEmpty() ? dirEntry.fileName() : relPath + "/" + dirEntry.fileName();
                    procDir(dirEntry.absoluteFilePath(), subRel);
                }
            }
        };
        procDir(rootPath, QString());
    }

    m_records = records;
    m_workerThread.quit();
    QMetaObject::invokeMethod(this, [this, successCount, failCount, skipCount]() {
        QStringList parts;
        if (successCount > 0) parts << QStringLiteral("成功 %1 个").arg(successCount);
        if (failCount > 0) parts << QStringLiteral("失败 %1 个").arg(failCount);
        if (skipCount > 0) parts << QStringLiteral("跳过 %1 个").arg(skipCount);

        if (parts.isEmpty()) {
            setStatusMessage(QStringLiteral("没有找到图片文件"));
            m_logger->info("处理完成: 没有找到图片文件");
        } else {
            setStatusMessage(parts.join("，"));
            m_logger->info(QString("处理完成: %1").arg(parts.join(", ")));
        }
        emit recordsChanged();
        emit hasRecordsChanged();
        setIsProcessing(false);
    }, Qt::QueuedConnection);
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
    if (m_workerRunning) {
        m_workerThread.quit();
        if (!m_workerThread.wait(3000)) {
            m_workerThread.terminate();
            m_workerThread.wait(3000);
        }
    }
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
