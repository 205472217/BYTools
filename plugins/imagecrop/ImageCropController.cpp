#include "ImageCropController.h"
#include "ImageCropPlugin.h"
#include "ImageCropSettings.h"
#include "Config.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QUrl>

ImageCropController::ImageCropController(PluginLogger *logger, ImageCropSettings *settings, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(settings)
{
}

// ── Getters / Setters ──────────────────────────────────────────────────

int ImageCropController::cropX() const { return m_cropX; }

void ImageCropController::setCropX(int x)
{
    if (m_cropX == x) return;
    m_cropX = x;
    emit cropXChanged();
}

int ImageCropController::cropY() const { return m_cropY; }

void ImageCropController::setCropY(int y)
{
    if (m_cropY == y) return;
    m_cropY = y;
    emit cropYChanged();
}

int ImageCropController::cropW() const { return m_cropW; }

void ImageCropController::setCropW(int w)
{
    if (m_cropW == w) return;
    m_cropW = w;
    emit cropWChanged();
}

int ImageCropController::cropH() const { return m_cropH; }

void ImageCropController::setCropH(int h)
{
    if (m_cropH == h) return;
    m_cropH = h;
    emit cropHChanged();
}

QString ImageCropController::statusMessage() const { return m_statusMessage; }
bool ImageCropController::hasRecords() const { return !m_records.isEmpty(); }

QVariantList ImageCropController::records() const
{
    QVariantList result;
    for (const auto &record : m_records) {
        QVariantMap map;
        map["originalPath"] = record.originalPath;
        map["newPath"] = record.newPath;
        map["originalName"] = record.originalName;
        map["newName"] = record.newName;
        map["cropW"] = record.cropW;
        map["cropH"] = record.cropH;
        map["success"] = record.success;
        map["status"] = record.status;
        result.append(map);
    }
    return result;
}

int ImageCropController::currentIndex() const { return m_currentIndex; }
int ImageCropController::currentFileCount() const { return m_imageFiles.size(); }

QString ImageCropController::currentFilePath() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_imageFiles.size()) {
        return m_imageFiles.at(m_currentIndex);
    }
    return {};
}

int ImageCropController::imageVersion() const
{
    return m_imageVersion;
}

bool ImageCropController::canRestoreCurrent() const
{
    return !m_backups.isEmpty();
}

// ── Image Scanning ─────────────────────────────────────────────────────

void ImageCropController::scanImages()
{
    QString rootPath = m_settings->rootPath();
    bool recursive = m_settings->recursive();

    m_backups.clear();
    m_imageFiles.clear();
    m_currentIndex = -1;

    if (rootPath.isEmpty() || !QDir(rootPath).exists()) {
        setStatusMessage(QStringLiteral("请选择有效的源文件夹"));
        m_logger->warn("扫描图片失败: 源文件夹无效");
        emit currentIndexChanged();
        emit currentFileCountChanged();
        emit currentFilePathChanged();
        return;
    }

    m_logger->info(QString("扫描图片目录: %1 (递归=%2)").arg(rootPath).arg(recursive));

    QDir dir(rootPath);
    QFileInfoList entries = dir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo &entry : entries) {
        if (isImageFile(entry.fileName())) {
            m_imageFiles.append(entry.absoluteFilePath());
        }
    }

    if (recursive) {
        QFileInfoList dirs = dir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &dirEntry : dirs) {
            QDir subDir(dirEntry.absoluteFilePath());
            QFileInfoList subEntries = subDir.entryInfoList(
                QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            for (const QFileInfo &subEntry : subEntries) {
                if (isImageFile(subEntry.fileName())) {
                    m_imageFiles.append(subEntry.absoluteFilePath());
                }
            }
        }
    }

    if (!m_imageFiles.isEmpty()) {
        m_currentIndex = 0;
        setStatusMessage(QStringLiteral("已扫描到 %1 张图片").arg(m_imageFiles.size()));
        m_logger->info(QString("扫描完成: 共 %1 张图片").arg(m_imageFiles.size()));
    } else {
        setStatusMessage(QStringLiteral("未找到图片文件"));
        m_logger->info("扫描完成: 未找到图片文件");
    }

    emit currentIndexChanged();
    emit currentFileCountChanged();
    emit currentFilePathChanged();
    emit canRestoreCurrentChanged();
}

// ── Navigation ─────────────────────────────────────────────────────────

bool ImageCropController::navigateNext()
{
    if (m_imageFiles.isEmpty()) return false;
    m_backups.clear();
    m_currentIndex = (m_currentIndex + 1) % m_imageFiles.size();
    emit currentIndexChanged();
    emit currentFilePathChanged();
    emit canRestoreCurrentChanged();
    return true;
}

bool ImageCropController::navigatePrev()
{
    if (m_imageFiles.isEmpty()) return false;
    m_backups.clear();
    m_currentIndex = (m_currentIndex - 1 + m_imageFiles.size()) % m_imageFiles.size();
    emit currentIndexChanged();
    emit currentFilePathChanged();
    emit canRestoreCurrentChanged();
    return true;
}

// ── Image Info ─────────────────────────────────────────────────────────

QVariantMap ImageCropController::getCurrentImageInfo() const
{
    QVariantMap info;
    QString path = currentFilePath();
    if (path.isEmpty()) return info;

    QFileInfo fi(path);
    info["name"] = fi.fileName();
    info["path"] = path;
    info["width"] = 0;
    info["height"] = 0;

    QImageReader reader(path);
    if (reader.canRead()) {
        QSize size = reader.size();
        if (size.isValid()) {
            info["width"] = size.width();
            info["height"] = size.height();
        }
    }

    return info;
}

QVariantList ImageCropController::getAllFilePaths() const
{
    QVariantList result;
    for (const QString &path : m_imageFiles) {
        result.append(path);
    }
    return result;
}

int ImageCropController::getFileCount() const
{
    return m_imageFiles.size();
}

// ── Crop Execution ─────────────────────────────────────────────────────

bool ImageCropController::executeCrop(int cropX, int cropY, int cropW, int cropH)
{
    QString rootPath = m_settings->rootPath();
    int outputMode = m_settings->outputMode();
    QString outputDir = m_settings->outputDir();
    QString suffix = buildCropSuffix();

    if (rootPath.isEmpty() || m_imageFiles.isEmpty() || m_currentIndex < 0) {
        setStatusMessage(QStringLiteral("没有可裁剪的图片"));
        m_logger->warn("裁剪失败: 没有可裁剪的图片");
        return false;
    }

    if (cropW <= 0 || cropH <= 0) {
        setStatusMessage(QStringLiteral("裁剪区域无效"));
        m_logger->warn("裁剪失败: 裁剪区域无效");
        return false;
    }

    setIsProcessing(true);

    QString srcPath = m_imageFiles.at(m_currentIndex);
    m_logger->info(QString("开始裁剪: %1 [%2,%3 %4x%5]").arg(srcPath).arg(cropX).arg(cropY).arg(cropW).arg(cropH));

    QImage image(srcPath);
    if (image.isNull()) {
        setStatusMessage(QStringLiteral("无法读取图片"));
        addRecord(srcPath, {}, cropW, cropH, false, QStringLiteral("失败：无法读取"));
        m_logger->error("裁剪失败: 无法读取图片 " + srcPath);
        setIsProcessing(false);
        return false;
    }

    // Clamp crop rectangle to image bounds
    int imgW = image.width();
    int imgH = image.height();
    int clampedX = qBound(0, cropX, imgW - 1);
    int clampedY = qBound(0, cropY, imgH - 1);
    int clampedW = qMin(cropW, imgW - clampedX);
    int clampedH = qMin(cropH, imgH - clampedY);

    if (clampedW <= 0 || clampedH <= 0) {
        setStatusMessage(QStringLiteral("裁剪区域超出图片范围"));
        m_logger->warn("裁剪失败: 裁剪区域超出图片范围");
        setIsProcessing(false);
        return false;
    }

    QFileInfo fi(srcPath);
    QString destPath;
    QString newName;

    if (outputMode == 0) {
        // Overwrite source file directly
        destPath = srcPath;
        newName = fi.fileName();
    } else {
        QString cropSuffix = suffix.isEmpty() ? "_cropped" : suffix;
        QString baseName = fi.completeBaseName();
        QString ext = QStringLiteral(".") + fi.suffix().toLower();
        newName = baseName + cropSuffix + ext;
        QString outRoot = outputDir.isEmpty()
            ? rootPath + "_cropped"
            : outputDir;
        QDir().mkpath(outRoot);
        destPath = outRoot + "/" + newName;
    }

    // Keep an in-memory backup before the file state changes
    if (m_backups.size() >= MAX_BACKUPS)
        m_backups.removeLast();
    m_backups.prepend(image.copy());

    QImage cropped = image.copy(clampedX, clampedY, clampedW, clampedH);
    bool ok = cropped.save(destPath);

    if (ok) {
        addRecord(srcPath, destPath, clampedW, clampedH, true, QStringLiteral("已裁剪"));
        setStatusMessage(QStringLiteral("裁剪成功：%1").arg(newName));
        m_logger->info(QString("裁剪成功: %1 → %2").arg(fi.fileName(), newName));

        // Bump version to force QML to reload preview (always needed after a crop)
        ++m_imageVersion;
        emit imageVersionChanged();
    } else {
        addRecord(srcPath, destPath, clampedW, clampedH, false, QStringLiteral("失败：保存失败"));
        setStatusMessage(QStringLiteral("裁剪失败：%1").arg(fi.fileName()));
        m_logger->error(QString("裁剪失败: %1 — 保存失败").arg(fi.fileName()));
    }

    emit recordsChanged();
    emit hasRecordsChanged();
    emit canRestoreCurrentChanged();
    setIsProcessing(false);
    return ok;
}

// ── Records & Reset ────────────────────────────────────────────────────

void ImageCropController::clearRecords()
{
    m_records.clear();
    emit recordsChanged();
    emit hasRecordsChanged();
    emit canRestoreCurrentChanged();
    setStatusMessage(QString());
}

void ImageCropController::restoreCurrentFile()
{
    QString curPath = currentFilePath();
    if (curPath.isEmpty() || m_backups.isEmpty()) {
        setStatusMessage(QStringLiteral("没有可还原的记录"));
        return;
    }

    QImage backup = m_backups.takeFirst();
    if (backup.save(curPath)) {
        ++m_imageVersion;
        emit imageVersionChanged();
        emit canRestoreCurrentChanged();
        setStatusMessage(QStringLiteral("已还原"));
        m_logger->info(QString("已还原: %1").arg(QFileInfo(curPath).fileName()));
    } else {
        setStatusMessage(QStringLiteral("还原失败"));
        m_logger->error(QString("还原失败: %1").arg(curPath));
    }
}

void ImageCropController::reset()
{
    cancel();
    m_backups.clear();
    m_records.clear();
    m_imageFiles.clear();
    m_currentIndex = -1;
    m_cropX = 0;
    m_cropY = 0;
    m_cropW = 0;
    m_cropH = 0;
    setStatusMessage(QString());

    emit recordsChanged();
    emit hasRecordsChanged();
    emit canRestoreCurrentChanged();
    emit cropXChanged();
    emit cropYChanged();
    emit cropWChanged();
    emit cropHChanged();
    emit currentIndexChanged();
    emit currentFileCountChanged();
    emit currentFilePathChanged();
}

void ImageCropController::resetCropRect()
{
    m_cropX = 0;
    m_cropY = 0;
    m_cropW = 0;
    m_cropH = 0;
}

// ── Private Helpers ────────────────────────────────────────────────────

bool ImageCropController::isProcessing() const
{
    return m_isProcessing;
}

void ImageCropController::cancel()
{
    if (m_isProcessing) {
        m_logger->info("图片裁剪已取消");
        setIsProcessing(false);
        setStatusMessage("已取消");
    }
}

void ImageCropController::setIsProcessing(bool processing)
{
    if (m_isProcessing == processing) return;
    m_isProcessing = processing;
    emit isProcessingChanged();
}

void ImageCropController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) return;
    m_statusMessage = message;
    emit statusMessageChanged();
}

bool ImageCropController::isImageFile(const QString &fileName) const
{
    QString ext = QStringLiteral(".") + QFileInfo(fileName).suffix().toLower();
    return m_imageExtensions.contains(ext);
}

void ImageCropController::addRecord(const QString &originalPath, const QString &newPath,
                                     int cropW, int cropH, bool success, const QString &status)
{
    CropRecord record;
    record.originalPath = originalPath;
    record.newPath = newPath;
    record.originalName = QFileInfo(originalPath).fileName();
    record.newName = newPath.isEmpty() ? QString() : QFileInfo(newPath).fileName();
    record.cropW = cropW;
    record.cropH = cropH;
    record.success = success;
    record.status = status;
    m_records.append(record);
}

QString ImageCropController::buildCropSuffix() const
{
    return QStringLiteral("_cropped");
}

// ── Crop Calculation Helpers (moved from QML) ──────────────────────────

double ImageCropController::calcEffectiveRatio() const
{
    int cropMode = m_settings->cropMode();
    int presetRatioIndex = m_settings->presetRatioIndex();
    bool usePresetRatio = m_settings->usePresetRatio();
    int customRatioW = m_settings->customRatioW();
    int customRatioH = m_settings->customRatioH();

    if (cropMode == 0) {
        if (usePresetRatio) {
            if (presetRatioIndex == 4)
                return 0;
            if (presetRatioIndex >= 0 && presetRatioIndex < RATIO_COUNT)
                return PRESET_RATIOS[presetRatioIndex][0] / PRESET_RATIOS[presetRatioIndex][1];
            return 1;
        }
        return (customRatioW > 0 && customRatioH > 0)
            ? static_cast<double>(customRatioW) / customRatioH : 1;
    }
    return 0;
}

QVariantMap ImageCropController::constrainToRatio(double rawW, double rawH) const
{
    constexpr int MIN_SIZE = 20;
    double ratio = calcEffectiveRatio();
    QVariantMap result;

    if (ratio <= 0) {
        result["w"] = qMax(MIN_SIZE, static_cast<int>(rawW));
        result["h"] = qMax(MIN_SIZE, static_cast<int>(rawH));
        return result;
    }

    double t = (rawW * ratio + rawH) / (ratio * ratio + 1);
    int nw = qRound(t * ratio);
    int nh = qRound(t);

    if (nw < MIN_SIZE) {
        nw = MIN_SIZE;
        nh = qRound(nw / ratio);
    }
    if (nh < MIN_SIZE) {
        nh = MIN_SIZE;
        nw = qRound(nh * ratio);
    }

    result["w"] = nw;
    result["h"] = nh;
    return result;
}

QVariantMap ImageCropController::calcDisplayDimensions(int containerW, int containerH, int srcW, int srcH)
{
    QVariantMap result;
    if (containerW <= 0 || containerH <= 0 || srcW <= 0 || srcH <= 0) {
        result["dispW"] = 0;
        result["dispH"] = 0;
        return result;
    }

    double scaleX = static_cast<double>(containerW) / srcW;
    double scaleY = static_cast<double>(containerH) / srcH;
    double s = qMin(scaleX, scaleY);

    result["dispW"] = qRound(srcW * s);
    result["dispH"] = qRound(srcH * s);
    return result;
}

QVariantMap ImageCropController::calcInitCropRect(int dispW, int dispH, int srcW, int srcH) const
{
    int cropMode = m_settings->cropMode();
    int targetWidth = m_settings->targetWidth();
    int targetHeight = m_settings->targetHeight();

    QVariantMap result;
    result["cropReady"] = false;
    if (dispW <= 0 || dispH <= 0)
        return result;

    int cropX = 0, cropY = 0, cropW = 0, cropH = 0;

    if (cropMode == 1) {
        double scaleX = static_cast<double>(dispW) / qMax(srcW, 1);
        double scaleY = static_cast<double>(dispH) / qMax(srcH, 1);
        int pw = qMin(static_cast<int>(targetWidth * scaleX), dispW);
        int ph = qMin(static_cast<int>(targetHeight * scaleY), dispH);
        cropW = pw;
        cropH = ph;
    } else {
        double r = calcEffectiveRatio();
        if (r > 0) {
            int fitW = static_cast<int>(dispH * r);
            if (fitW <= dispW) {
                cropW = fitW;
                cropH = dispH;
            } else {
                cropW = dispW;
                cropH = static_cast<int>(dispW / r);
            }
        } else {
            cropW = dispW;
            cropH = dispH;
        }
    }

    cropX = (dispW - cropW) / 2;
    cropY = (dispH - cropH) / 2;

    result["x"] = cropX;
    result["y"] = cropY;
    result["w"] = cropW;
    result["h"] = cropH;
    result["cropReady"] = true;
    return result;
}

QVariantMap ImageCropController::clampCropRect(int cropX, int cropY, int cropW, int cropH, int dispW, int dispH)
{
    QVariantMap result;
    constexpr int MIN_SIZE = 20;

    cropW = qMax(MIN_SIZE, qMin(cropW, dispW));
    cropH = qMax(MIN_SIZE, qMin(cropH, dispH));
    cropX = qMax(0, qMin(cropX, dispW - cropW));
    cropY = qMax(0, qMin(cropY, dispH - cropH));

    result["x"] = cropX;
    result["y"] = cropY;
    result["w"] = cropW;
    result["h"] = cropH;
    return result;
}

void ImageCropController::syncCropToController(int cropX, int cropY, int cropW, int cropH, int dispW, int dispH, int srcW, int srcH)
{
    int cropMode = m_settings->cropMode();
    int targetWidth = m_settings->targetWidth();
    int targetHeight = m_settings->targetHeight();

    double scaleX = srcW / static_cast<double>(qMax(dispW, 1));
    double scaleY = srcH / static_cast<double>(qMax(dispH, 1));

    setCropX(qRound(cropX * scaleX));
    setCropY(qRound(cropY * scaleY));

    if (cropMode == 1) {
        setCropW(targetWidth);
        setCropH(targetHeight);
    } else {
        setCropW(qRound(cropW * scaleX));
        setCropH(qRound(cropH * scaleY));
    }
}

int ImageCropController::hitTest(int mx, int my, int cropX, int cropY, int cropW, int cropH, int cornerHitSize) const
{
    // Corners
    if (qAbs(mx - cropX) < cornerHitSize && qAbs(my - cropY) < cornerHitSize)
        return 2;
    if (qAbs(mx - (cropX + cropW)) < cornerHitSize && qAbs(my - cropY) < cornerHitSize)
        return 3;
    if (qAbs(mx - cropX) < cornerHitSize && qAbs(my - (cropY + cropH)) < cornerHitSize)
        return 4;
    if (qAbs(mx - (cropX + cropW)) < cornerHitSize && qAbs(my - (cropY + cropH)) < cornerHitSize)
        return 5;

    // Edge midpoints (a narrower hit zone so corners take priority)
    int edgeHit = cornerHitSize / 2;
    bool nearTop = qAbs(my - cropY) < edgeHit;
    bool nearBottom = qAbs(my - (cropY + cropH)) < edgeHit;
    bool nearLeft = qAbs(mx - cropX) < edgeHit;
    bool nearRight = qAbs(mx - (cropX + cropW)) < edgeHit;

    if (nearTop && mx > cropX + edgeHit && mx < cropX + cropW - edgeHit)
        return 6;   // Top edge
    if (nearBottom && mx > cropX + edgeHit && mx < cropX + cropW - edgeHit)
        return 7;   // Bottom edge
    if (nearLeft && my > cropY + edgeHit && my < cropY + cropH - edgeHit)
        return 8;   // Left edge
    if (nearRight && my > cropY + edgeHit && my < cropY + cropH - edgeHit)
        return 9;   // Right edge

    // Interior
    if (mx >= cropX && mx <= cropX + cropW && my >= cropY && my <= cropY + cropH)
        return 1;
    return 0;
}

QString ImageCropController::extractFileName(const QString &filePath)
{
    if (filePath.isEmpty())
        return {};
    QString p = QString(filePath).replace(QLatin1Char('\\'), QLatin1Char('/'));
    int idx = p.lastIndexOf(QLatin1Char('/'));
    if (idx >= 0)
        return p.mid(idx + 1);
    return p;
}
