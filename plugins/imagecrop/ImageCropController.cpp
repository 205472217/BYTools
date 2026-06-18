#include "ImageCropController.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QUrl>

ImageCropController::ImageCropController(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
{
}

// ── Getters / Setters ──────────────────────────────────────────────────

QString ImageCropController::rootPath() const { return m_rootPath; }

void ImageCropController::setRootPath(const QString &path)
{
    QString normalizedPath = path;
    const QUrl url(path);
    if (url.isLocalFile()) {
        normalizedPath = url.toLocalFile();
    }
    if (m_rootPath == normalizedPath) return;
    m_rootPath = normalizedPath;
    emit rootPathChanged();
}

bool ImageCropController::recursive() const { return m_recursive; }

void ImageCropController::setRecursive(bool recursive)
{
    if (m_recursive == recursive) return;
    m_recursive = recursive;
    emit recursiveChanged();
}

int ImageCropController::cropMode() const { return m_cropMode; }

void ImageCropController::setCropMode(int mode)
{
    if (m_cropMode == mode) return;
    m_cropMode = mode;
    emit cropModeChanged();
}

int ImageCropController::presetRatioIndex() const { return m_presetRatioIndex; }

void ImageCropController::setPresetRatioIndex(int index)
{
    if (m_presetRatioIndex == index) return;
    m_presetRatioIndex = index;
    emit presetRatioIndexChanged();
}

bool ImageCropController::usePresetRatio() const { return m_usePresetRatio; }

void ImageCropController::setUsePresetRatio(bool use)
{
    if (m_usePresetRatio == use) return;
    m_usePresetRatio = use;
    emit usePresetRatioChanged();
}

int ImageCropController::customRatioW() const { return m_customRatioW; }

void ImageCropController::setCustomRatioW(int w)
{
    if (w < 1) w = 1;
    if (m_customRatioW == w) return;
    m_customRatioW = w;
    emit customRatioWChanged();
}

int ImageCropController::customRatioH() const { return m_customRatioH; }

void ImageCropController::setCustomRatioH(int h)
{
    if (h < 1) h = 1;
    if (m_customRatioH == h) return;
    m_customRatioH = h;
    emit customRatioHChanged();
}

int ImageCropController::targetWidth() const { return m_targetWidth; }

void ImageCropController::setTargetWidth(int w)
{
    if (w < 1) w = 1;
    if (m_targetWidth == w) return;
    m_targetWidth = w;
    emit targetWidthChanged();
}

int ImageCropController::targetHeight() const { return m_targetHeight; }

void ImageCropController::setTargetHeight(int h)
{
    if (h < 1) h = 1;
    if (m_targetHeight == h) return;
    m_targetHeight = h;
    emit targetHeightChanged();
}

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

int ImageCropController::outputMode() const { return m_outputMode; }

void ImageCropController::setOutputMode(int mode)
{
    if (m_outputMode == mode) return;
    m_outputMode = mode;
    emit outputModeChanged();
}

QString ImageCropController::outputDir() const { return m_outputDir; }

void ImageCropController::setOutputDir(const QString &dir)
{
    if (m_outputDir == dir) return;
    m_outputDir = dir;
    emit outputDirChanged();
}

QString ImageCropController::suffix() const { return m_suffix; }

void ImageCropController::setSuffix(const QString &suffix)
{
    if (m_suffix == suffix) return;
    m_suffix = suffix;
    emit suffixChanged();
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

// ── Image Scanning ─────────────────────────────────────────────────────

void ImageCropController::scanImages()
{
    m_imageFiles.clear();
    m_currentIndex = -1;

    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        setStatusMessage(QStringLiteral("请选择有效的源文件夹"));
        m_logger->warn("扫描图片失败: 源文件夹无效");
        emit currentIndexChanged();
        emit currentFileCountChanged();
        emit currentFilePathChanged();
        return;
    }

    m_logger->info(QString("扫描图片目录: %1 (递归=%2)").arg(m_rootPath).arg(m_recursive));

    QDir dir(m_rootPath);
    QFileInfoList entries = dir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo &entry : entries) {
        if (isImageFile(entry.fileName())) {
            m_imageFiles.append(entry.absoluteFilePath());
        }
    }

    if (m_recursive) {
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
}

// ── Navigation ─────────────────────────────────────────────────────────

bool ImageCropController::navigateNext()
{
    if (m_imageFiles.isEmpty()) return false;
    m_currentIndex = (m_currentIndex + 1) % m_imageFiles.size();
    emit currentIndexChanged();
    emit currentFilePathChanged();
    return true;
}

bool ImageCropController::navigatePrev()
{
    if (m_imageFiles.isEmpty()) return false;
    m_currentIndex = (m_currentIndex - 1 + m_imageFiles.size()) % m_imageFiles.size();
    emit currentIndexChanged();
    emit currentFilePathChanged();
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
    if (m_rootPath.isEmpty() || m_imageFiles.isEmpty() || m_currentIndex < 0) {
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

    if (m_outputMode == 0) {
        // Overwrite source file directly
        destPath = srcPath;
        newName = fi.fileName();
    } else {
        QString cropSuffix = buildCropSuffix();
        QString baseName = fi.completeBaseName();
        QString ext = QStringLiteral(".") + fi.suffix().toLower();
        newName = baseName + cropSuffix + ext;
        QString outRoot = m_outputDir.isEmpty()
            ? m_rootPath + "_cropped"
            : m_outputDir;
        QDir().mkpath(outRoot);
        destPath = outRoot + "/" + newName;
    }

    QImage cropped = image.copy(clampedX, clampedY, clampedW, clampedH);
    bool ok = cropped.save(destPath);

    if (ok) {
        addRecord(srcPath, destPath, clampedW, clampedH, true, QStringLiteral("已裁剪"));
        setStatusMessage(QStringLiteral("裁剪成功：%1").arg(newName));
        m_logger->info(QString("裁剪成功: %1 → %2").arg(fi.fileName(), newName));
    } else {
        addRecord(srcPath, destPath, clampedW, clampedH, false, QStringLiteral("失败：保存失败"));
        setStatusMessage(QStringLiteral("裁剪失败：%1").arg(fi.fileName()));
        m_logger->error(QString("裁剪失败: %1 — 保存失败").arg(fi.fileName()));
    }

    emit recordsChanged();
    emit hasRecordsChanged();
    setIsProcessing(false);
    return ok;
}

// ── Records & Reset ────────────────────────────────────────────────────

void ImageCropController::clearRecords()
{
    m_records.clear();
    emit recordsChanged();
    emit hasRecordsChanged();
    setStatusMessage(QString());
}

void ImageCropController::reset()
{
    cancel();
    m_records.clear();
    m_imageFiles.clear();
    m_currentIndex = -1;
    m_rootPath.clear();
    m_recursive = false;
    m_cropMode = 0;
    m_presetRatioIndex = 0;
    m_usePresetRatio = true;
    m_customRatioW = 1;
    m_customRatioH = 1;
    m_targetWidth = 800;
    m_targetHeight = 600;
    m_cropX = 0;
    m_cropY = 0;
    m_cropW = 0;
    m_cropH = 0;
    m_outputMode = 0;
    m_outputDir.clear();
    m_suffix = "_cropped";
    setStatusMessage(QString());

    emit recordsChanged();
    emit hasRecordsChanged();
    emit rootPathChanged();
    emit recursiveChanged();
    emit cropModeChanged();
    emit presetRatioIndexChanged();
    emit usePresetRatioChanged();
    emit customRatioWChanged();
    emit customRatioHChanged();
    emit targetWidthChanged();
    emit targetHeightChanged();
    emit cropXChanged();
    emit cropYChanged();
    emit cropWChanged();
    emit cropHChanged();
    emit outputModeChanged();
    emit outputDirChanged();
    emit suffixChanged();
    emit currentIndexChanged();
    emit currentFileCountChanged();
    emit currentFilePathChanged();
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
    if (m_suffix.isEmpty()) return "_cropped";
    return m_suffix;
}

// ── Crop Calculation Helpers (moved from QML) ──────────────────────────

double ImageCropController::calcEffectiveRatio() const
{
    if (m_cropMode == 0) {
        if (m_usePresetRatio) {
            if (m_presetRatioIndex == 4)
                return 0;
            if (m_presetRatioIndex >= 0 && m_presetRatioIndex < RATIO_COUNT)
                return PRESET_RATIOS[m_presetRatioIndex][0] / PRESET_RATIOS[m_presetRatioIndex][1];
            return 1;
        }
        return (m_customRatioW > 0 && m_customRatioH > 0)
            ? static_cast<double>(m_customRatioW) / m_customRatioH : 1;
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
    QVariantMap result;
    result["cropReady"] = false;
    if (dispW <= 0 || dispH <= 0)
        return result;

    int cropX = 0, cropY = 0, cropW = 0, cropH = 0;

    if (m_cropMode == 1) {
        double scaleX = static_cast<double>(dispW) / qMax(srcW, 1);
        double scaleY = static_cast<double>(dispH) / qMax(srcH, 1);
        int pw = qMin(static_cast<int>(m_targetWidth * scaleX), dispW);
        int ph = qMin(static_cast<int>(m_targetHeight * scaleY), dispH);
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
    double scaleX = srcW / static_cast<double>(qMax(dispW, 1));
    double scaleY = srcH / static_cast<double>(qMax(dispH, 1));

    setCropX(qRound(cropX * scaleX));
    setCropY(qRound(cropY * scaleY));

    if (m_cropMode == 1) {
        setCropW(m_targetWidth);
        setCropH(m_targetHeight);
    } else {
        setCropW(qRound(cropW * scaleX));
        setCropH(qRound(cropH * scaleY));
    }
}

int ImageCropController::hitTest(int mx, int my, int cropX, int cropY, int cropW, int cropH, int cornerHitSize) const
{
    if (qAbs(mx - cropX) < cornerHitSize && qAbs(my - cropY) < cornerHitSize)
        return 2;
    if (qAbs(mx - (cropX + cropW)) < cornerHitSize && qAbs(my - cropY) < cornerHitSize)
        return 3;
    if (qAbs(mx - cropX) < cornerHitSize && qAbs(my - (cropY + cropH)) < cornerHitSize)
        return 4;
    if (qAbs(mx - (cropX + cropW)) < cornerHitSize && qAbs(my - (cropY + cropH)) < cornerHitSize)
        return 5;
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