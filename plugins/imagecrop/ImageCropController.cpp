#include "ImageCropController.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QUrl>

ImageCropController::ImageCropController(QObject *parent)
    : QObject(parent)
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
        emit currentIndexChanged();
        emit currentFileCountChanged();
        emit currentFilePathChanged();
        return;
    }

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
    } else {
        setStatusMessage(QStringLiteral("未找到图片文件"));
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
        return false;
    }

    if (cropW <= 0 || cropH <= 0) {
        setStatusMessage(QStringLiteral("裁剪区域无效"));
        return false;
    }

    QString srcPath = m_imageFiles.at(m_currentIndex);
    QImage image(srcPath);
    if (image.isNull()) {
        setStatusMessage(QStringLiteral("无法读取图片"));
        addRecord(srcPath, {}, cropW, cropH, false, QStringLiteral("失败：无法读取"));
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
    } else {
        addRecord(srcPath, destPath, clampedW, clampedH, false, QStringLiteral("失败：保存失败"));
        setStatusMessage(QStringLiteral("裁剪失败：%1").arg(fi.fileName()));
    }

    emit recordsChanged();
    emit hasRecordsChanged();
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