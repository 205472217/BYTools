#include "ImageCropSettings.h"
#include "ImageCropPlugin.h"
#include "SettingsHelper.h"
#include <QDir>

ImageCropSettings::ImageCropSettings(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

QString ImageCropSettings::rootPath() const { return m_rootPath; }
bool ImageCropSettings::recursive() const { return m_recursive; }
int ImageCropSettings::cropMode() const { return m_cropMode; }
int ImageCropSettings::presetRatioIndex() const { return m_presetRatioIndex; }
bool ImageCropSettings::usePresetRatio() const { return m_usePresetRatio; }
int ImageCropSettings::customRatioW() const { return m_customRatioW; }
int ImageCropSettings::customRatioH() const { return m_customRatioH; }
int ImageCropSettings::targetWidth() const { return m_targetWidth; }
int ImageCropSettings::targetHeight() const { return m_targetHeight; }
int ImageCropSettings::outputMode() const { return m_outputMode; }
QString ImageCropSettings::outputDir() const { return m_outputDir; }

void ImageCropSettings::setRootPath(const QString &path)
{
    if (m_rootPath != path) {
        m_rootPath = path;
        emit rootPathChanged();
        saveSettings();
    }
}
void ImageCropSettings::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        emit recursiveChanged();
        saveSettings();
    }
}
void ImageCropSettings::setCropMode(int mode)
{
    if (m_cropMode != mode) {
        m_cropMode = mode;
        emit cropModeChanged();
    }
}
void ImageCropSettings::setPresetRatioIndex(int index)
{
    if (m_presetRatioIndex != index) {
        m_presetRatioIndex = index;
        emit presetRatioIndexChanged();
    }
}
void ImageCropSettings::setUsePresetRatio(bool use)
{
    if (m_usePresetRatio != use) {
        m_usePresetRatio = use;
        emit usePresetRatioChanged();
    }
}
void ImageCropSettings::setCustomRatioW(int w)
{
    if (w < 1) w = 1;
    if (m_customRatioW != w) {
        m_customRatioW = w;
        emit customRatioWChanged();
    }
}
void ImageCropSettings::setCustomRatioH(int h)
{
    if (h < 1) h = 1;
    if (m_customRatioH != h) {
        m_customRatioH = h;
        emit customRatioHChanged();
    }
}
void ImageCropSettings::setTargetWidth(int w)
{
    if (w < 1) w = 1;
    if (m_targetWidth != w) {
        m_targetWidth = w;
        emit targetWidthChanged();
    }
}
void ImageCropSettings::setTargetHeight(int h)
{
    if (h < 1) h = 1;
    if (m_targetHeight != h) {
        m_targetHeight = h;
        emit targetHeightChanged();
    }
}
void ImageCropSettings::setOutputMode(int mode)
{
    if (m_outputMode != mode) {
        m_outputMode = mode;
        emit outputModeChanged();
        saveSettings();
    }
}
void ImageCropSettings::setOutputDir(const QString &dir)
{
    if (m_outputDir != dir) {
        m_outputDir = dir;
        emit outputDirChanged();
        saveSettings();
    }
}

void ImageCropSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings(ImageCropPlugin::PluginKey);
    m_rootPath = s.value("rootPath").toString();
    m_recursive = s.value("recursive", false).toBool();
    m_outputMode = s.value("outputMode", 0).toInt();
    m_outputDir = s.value("outputDir").toString();
}
void ImageCropSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings(ImageCropPlugin::PluginKey);
    s.setValue("rootPath", m_rootPath);
    s.setValue("recursive", m_recursive);
    s.setValue("outputMode", m_outputMode);
    s.setValue("outputDir", m_outputDir);
    s.sync();
}
