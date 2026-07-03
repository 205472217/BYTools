#include "ImageConverterController.h"
#include "ImageConverterPlugin.h"
#include "ImageConverterSettings.h"
#include "Config.h"
#include "Logger.h"
#include "GlobalConfig.h"
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

QString ImageConverterController::createBackup(const QString &filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists() || fi.size() == 0)
        return {};

    QString dir = GlobalConfig::backupPath() + "/" + ImageConverterPlugin::PluginKey;
    QDir().mkpath(dir);

    QString name = QString::number(qHash(fi.absoluteFilePath()), 16)
        + "_" + fi.fileName();
    QString dest = dir + "/" + name;

    if (QFile::exists(dest))
        QFile::remove(dest);

    if (QFile::copy(filePath, dest))
        return dest;
    return {};
}

QString ImageConverterController::statusMessage() const { return m_statusMessage; }
QString ImageConverterController::sourceFile() const { return m_sourceFile; }
void ImageConverterController::setSourceFile(const QString &path)
{
    if (m_sourceFile != path) {
        m_sourceFile = path;
        emit sourceFileChanged();
    }
}
bool ImageConverterController::hasRecords() const
{
    QMutexLocker locker(&m_recordsMutex);
    return !m_records.isEmpty();
}

QVariantList ImageConverterController::records() const
{
    QMutexLocker locker(&m_recordsMutex);
    QVariantList result;
    for (const auto &record : m_records) {
        QVariantMap map;
        map["originalPath"] = record.originalPath;
        map["newPath"] = record.newPath;
        map["originalName"] = record.originalName;
        map["newName"] = record.newName;
        map["formatTag"] = record.formatTag;
        map["success"] = record.success;
        map["restorable"] = record.restorable;
        map["status"] = record.status;
        result.append(map);
    }
    return result;
}

// === Config properties (delegated to ImageConverterSettings) ===
QString ImageConverterController::rootPath() const { return m_settings->rootPath(); }
void ImageConverterController::setRootPath(const QString &path) {
    if (m_settings->rootPath() != path) {
        m_settings->setRootPath(path);
        emit rootPathChanged();
    }
}
int ImageConverterController::targetFormat() const { return m_settings->targetFormat(); }
void ImageConverterController::setTargetFormat(int format) {
    if (m_settings->targetFormat() != format) {
        m_settings->setTargetFormat(format);
        emit targetFormatChanged();
    }
}
int ImageConverterController::quality() const { return m_settings->quality(); }
void ImageConverterController::setQuality(int quality) {
    if (m_settings->quality() != quality) {
        m_settings->setQuality(quality);
        emit qualityChanged();
    }
}
QString ImageConverterController::bgColor() const { return m_settings->bgColor(); }
void ImageConverterController::setBgColor(const QString &color) {
    if (m_settings->bgColor() != color) {
        m_settings->setBgColor(color);
        emit bgColorChanged();
    }
}
int ImageConverterController::outputMode() const { return m_settings->outputMode(); }
void ImageConverterController::setOutputMode(int mode) {
    if (m_settings->outputMode() != mode) {
        m_settings->setOutputMode(mode);
        emit outputModeChanged();
    }
}
QString ImageConverterController::outputDir() const { return m_settings->outputDir(); }
void ImageConverterController::setOutputDir(const QString &dir) {
    if (m_settings->outputDir() != dir) {
        m_settings->setOutputDir(dir);
        emit outputDirChanged();
    }
}
bool ImageConverterController::recursive() const { return m_settings->recursive(); }
void ImageConverterController::setRecursive(bool recursive) {
    if (m_settings->recursive() != recursive) {
        m_settings->setRecursive(recursive);
        emit recursiveChanged();
    }
}
bool ImageConverterController::convertEnabled() const { return m_settings->convertEnabled(); }
void ImageConverterController::setConvertEnabled(bool enabled) {
    if (m_settings->convertEnabled() != enabled) {
        m_settings->setConvertEnabled(enabled);
        emit convertEnabledChanged();
    }
}
bool ImageConverterController::resizeEnabled() const { return m_settings->resizeEnabled(); }
void ImageConverterController::setResizeEnabled(bool enabled) {
    if (m_settings->resizeEnabled() != enabled) {
        m_settings->setResizeEnabled(enabled);
        emit resizeEnabledChanged();
    }
}
int ImageConverterController::resizeMode() const { return m_settings->resizeMode(); }
void ImageConverterController::setResizeMode(int mode) {
    if (m_settings->resizeMode() != mode) {
        m_settings->setResizeMode(mode);
        emit resizeModeChanged();
    }
}
double ImageConverterController::resizeRatio() const { return m_settings->resizeRatio(); }
void ImageConverterController::setResizeRatio(double ratio) {
    if (m_settings->resizeRatio() != ratio) {
        m_settings->setResizeRatio(ratio);
        emit resizeRatioChanged();
    }
}
int ImageConverterController::resizeWidth() const { return m_settings->resizeWidth(); }
void ImageConverterController::setResizeWidth(int w) {
    if (m_settings->resizeWidth() != w) {
        m_settings->setResizeWidth(w);
        emit resizeWidthChanged();
    }
}
int ImageConverterController::resizeHeight() const { return m_settings->resizeHeight(); }
void ImageConverterController::setResizeHeight(int h) {
    if (m_settings->resizeHeight() != h) {
        m_settings->setResizeHeight(h);
        emit resizeHeightChanged();
    }
}
int ImageConverterController::mode() const { return m_settings->mode(); }
void ImageConverterController::setMode(int mode) {
    if (m_settings->mode() != mode) {
        m_settings->setMode(mode);
        emit modeChanged();
    }
}

void ImageConverterController::executeConvert()
{
    bool isSingle = (m_settings->mode() == 0);

    if (isSingle) {
        if (m_sourceFile.isEmpty() || !QFileInfo::exists(m_sourceFile)) {
            setStatusMessage(QStringLiteral("请选择有效的图片文件"));
            m_logger->warn("转换失败: 文件无效");
            return;
        }
        if (!isImageFile(QFileInfo(m_sourceFile).fileName())) {
            setStatusMessage(QStringLiteral("请选择图片文件"));
            m_logger->warn("转换失败: 不支持的文件格式");
            return;
        }
    } else {
        QString rootPath = m_settings->rootPath();
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
    QString srcDisplay = isSingle ? m_sourceFile : m_settings->rootPath();
    m_logger->info(QString("源路径: %1, 单文件=%2, 格式转换=%3, 宽高缩放=%4, 递归=%5")
        .arg(srcDisplay).arg(isSingle).arg(doConvert).arg(doResize).arg(recursive));
    emit logMessage(QString("源路径: %1").arg(srcDisplay));
    if (recursive)
        emit logMessage("  启用了递归查找，正在遍历子目录...");

    for (const auto &record : m_records)
        if (!record.backupPath.isEmpty()) QFile::remove(record.backupPath);
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

    // 同格式+无缩放+替换原文件 → 无需任何操作
    if (doConvert && srcExtDot == targetExt && !doResize && outputMode == 0) {
        records.append({
            entry.absoluteFilePath(), entry.absoluteFilePath(),
            entry.fileName(), entry.fileName(),
            formatTagForExt(srcExt), true, QStringLiteral("已跳过")
        });
        skipCount++;
        return;
    }

    // 覆盖原文件前先备份
    QString backupPath;
    bool backedUp = false;
    if (outputMode == 0) {
        backupPath = createBackup(entry.absoluteFilePath());
        backedUp = !backupPath.isEmpty();
        if (!backedUp)
            emit logMessage(QString("  [警告] %1 — 备份失败，将无法还原").arg(entry.fileName()));
    }

    if (doConvert && srcExtDot == targetExt && !doResize) {
        // outputMode == 1 同格式复制
        if (QFile::copy(entry.absoluteFilePath(), destPath)) {
            records.append({
                entry.absoluteFilePath(), destPath,
                entry.fileName(), QFileInfo(destPath).fileName(),
                formatTagForExt(srcExt), true, QStringLiteral("已复制"), backedUp, backupPath
            });
            successCount++;
            emit logMessage(QString("  [复制] %1 → %2").arg(entry.fileName(), QFileInfo(destPath).fileName()));
        } else if (QFileInfo::exists(destPath)) {
            if (!backupPath.isEmpty()) QFile::remove(backupPath);
            records.append({
                entry.absoluteFilePath(), destPath,
                entry.fileName(), QFileInfo(destPath).fileName(),
                formatTagForExt(srcExt), true, QStringLiteral("已存在")
            });
            successCount++;
            emit logMessage(QString("  [跳过] %1 — 输出文件已存在").arg(entry.fileName()));
        } else {
            if (!backupPath.isEmpty()) QFile::remove(backupPath);
            records.append({
                entry.absoluteFilePath(), destPath,
                entry.fileName(), QFileInfo(destPath).fileName(),
                formatTagForExt(srcExt), false, QStringLiteral("失败：复制失败")
            });
            failCount++;
            emit logMessage(QString("  [失败] %1 — 复制失败").arg(entry.fileName()));
        }
        return;
    }

    QImage image;
    if (entry.size() > 0)
        image = QImage(entry.absoluteFilePath());

    if (image.isNull()) {
        if (!backupPath.isEmpty()) QFile::remove(backupPath);
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
            image = image.scaled(resizeWidth, resizeHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
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
            formatTagForExt(srcExt), true, QStringLiteral("已处理"), backedUp, backupPath
        });
        successCount++;
        emit logMessage(QString("  [处理] %1 → %2").arg(entry.fileName(), QFileInfo(destPath).fileName()));
    } else {
        if (!backupPath.isEmpty()) QFile::remove(backupPath);
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
    bool isSingle = (m_settings->mode() == 0);
    QString rootPath = isSingle ? m_sourceFile : m_settings->rootPath();
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

        // 逐条追加到 m_records，让 QML 能实时看到每条记录
        if (!records.isEmpty()) {
            ConvertRecord rec = records.last();
            records.clear();
            {
                QMutexLocker locker(&m_recordsMutex);
                m_records.append(rec);
            }
            QMetaObject::invokeMethod(this, [this]() {
                emit recordsChanged();
                emit hasRecordsChanged();
            }, Qt::QueuedConnection);
        }
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
        setIsProcessing(false);
    }, Qt::QueuedConnection);
}

void ImageConverterController::clearRecords()
{
    for (const auto &record : m_records)
        if (!record.backupPath.isEmpty()) QFile::remove(record.backupPath);
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
    if (!record.success || !record.restorable)
        return;

    if (record.backupPath.isEmpty() || !QFileInfo::exists(record.backupPath)) {
        record.status = QStringLiteral("失败：备份文件不存在");
        record.success = false;
        record.restorable = false;
        setStatusMessage(QStringLiteral("还原失败：备份文件不存在"));
        m_logger->warn(QString("还原失败: %1 — 备份文件不存在").arg(record.originalName));
        emit recordsChanged();
        return;
    }

    // 1. 先确保目标目录存在
    QFileInfo origFi(record.originalPath);
    QDir().mkpath(origFi.absolutePath());

    // 2. 如果输出文件与源文件不同，删除输出文件
    if (record.newPath != record.originalPath)
        QFile::remove(record.newPath);

    // 3. 删除当前源文件（可能是已转换的版本）
    QFile::remove(record.originalPath);

    // 4. 从备份还原
    if (QFile::copy(record.backupPath, record.originalPath)) {
        // 5. 删除备份
        QFile::remove(record.backupPath);
        record.backupPath.clear();
        record.status = QStringLiteral("已还原");
        record.success = false;
        record.restorable = false;
        setStatusMessage(QStringLiteral("已还原：%1").arg(record.originalName));
        m_logger->info(QString("已还原: %1").arg(record.originalName));
    } else {
        record.status = QStringLiteral("失败：还原失败");
        record.success = false;
        record.restorable = false;
        setStatusMessage(QStringLiteral("还原失败：%1").arg(record.originalName));
        m_logger->error(QString("还原失败: %1 — 备份复制失败").arg(record.originalName));
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
        if (!record.success || !record.restorable)
            continue;

        if (record.backupPath.isEmpty() || !QFileInfo::exists(record.backupPath)) {
            record.status = QStringLiteral("失败：备份文件不存在");
            record.success = false;
            failCount++;
            continue;
        }

        QFileInfo origFi(record.originalPath);
        QDir().mkpath(origFi.absolutePath());

        if (record.newPath != record.originalPath)
            QFile::remove(record.newPath);

        QFile::remove(record.originalPath);

        if (QFile::copy(record.backupPath, record.originalPath)) {
            QFile::remove(record.backupPath);
            record.backupPath.clear();
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
    for (const auto &record : m_records)
        if (!record.backupPath.isEmpty()) QFile::remove(record.backupPath);
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
