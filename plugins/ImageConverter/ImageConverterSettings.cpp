#include "ImageConverterSettings.h"
#include "ImageConverterPlugin.h"
#include "SettingsHelper.h"
#include <QDir>

ImageConverterSettings::ImageConverterSettings(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

QString ImageConverterSettings::rootPath() const { return m_rootPath; }
int ImageConverterSettings::targetFormat() const { return m_targetFormat; }
int ImageConverterSettings::quality() const { return m_quality; }
QString ImageConverterSettings::bgColor() const { return m_bgColor; }
int ImageConverterSettings::outputMode() const { return m_outputMode; }
QString ImageConverterSettings::outputDir() const { return m_outputDir; }
bool ImageConverterSettings::recursive() const { return m_recursive; }
bool ImageConverterSettings::convertEnabled() const { return m_convertEnabled; }
bool ImageConverterSettings::resizeEnabled() const { return m_resizeEnabled; }
int ImageConverterSettings::resizeMode() const { return m_resizeMode; }
double ImageConverterSettings::resizeRatio() const { return m_resizeRatio; }
int ImageConverterSettings::resizeWidth() const { return m_resizeWidth; }
int ImageConverterSettings::resizeHeight() const { return m_resizeHeight; }
int ImageConverterSettings::mode() const { return m_mode; }

void ImageConverterSettings::setRootPath(const QString &path)
{
    if (m_rootPath != path) {
        m_rootPath = path;
        saveSettings();
    }
}
void ImageConverterSettings::setTargetFormat(int format)
{
    if (m_targetFormat != format) {
        m_targetFormat = format;
    }
}
void ImageConverterSettings::setQuality(int quality)
{
    if (m_quality != quality) {
        m_quality = quality;
    }
}
void ImageConverterSettings::setBgColor(const QString &color)
{
    if (m_bgColor != color) {
        m_bgColor = color;
    }
}
void ImageConverterSettings::setOutputMode(int mode)
{
    if (m_outputMode != mode) {
        m_outputMode = mode;
        saveSettings();
    }
}
void ImageConverterSettings::setOutputDir(const QString &dir)
{
    if (m_outputDir != dir) {
        m_outputDir = dir;
        saveSettings();
    }
}
void ImageConverterSettings::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        saveSettings();
    }
}
void ImageConverterSettings::setConvertEnabled(bool enabled)
{
    if (m_convertEnabled != enabled) {
        m_convertEnabled = enabled;
        saveSettings();
    }
}
void ImageConverterSettings::setResizeEnabled(bool enabled)
{
    if (m_resizeEnabled != enabled) {
        m_resizeEnabled = enabled;
        saveSettings();
    }
}
void ImageConverterSettings::setResizeMode(int mode)
{
    if (m_resizeMode != mode) {
        m_resizeMode = mode;
        saveSettings();
    }
}
void ImageConverterSettings::setResizeRatio(double ratio)
{
    if (qFuzzyCompare(m_resizeRatio, ratio))
        return;
    m_resizeRatio = ratio;

    saveSettings();
}
void ImageConverterSettings::setResizeWidth(int w)
{
    if (m_resizeWidth != w) {
        m_resizeWidth = w;
        saveSettings();
    }
}
void ImageConverterSettings::setResizeHeight(int h)
{
    if (m_resizeHeight != h) {
        m_resizeHeight = h;
        saveSettings();
    }
}
void ImageConverterSettings::setMode(int mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        saveSettings();
    }
}

void ImageConverterSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings(ImageConverterPlugin::PluginKey);
    m_rootPath = s.value("rootPath").toString();
    m_outputMode = s.value("outputMode", 0).toInt();
    m_outputDir = s.value("outputDir").toString();
    m_recursive = s.value("recursive", false).toBool();
    m_resizeMode = s.value("resizeMode", 0).toInt();
    m_resizeRatio = s.value("resizeRatio", 0.5).toDouble();
    m_resizeWidth = s.value("resizeWidth", 1920).toInt();
    m_resizeHeight = s.value("resizeHeight", 1080).toInt();
    m_mode = s.value("mode", 0).toInt();
}
void ImageConverterSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings(ImageConverterPlugin::PluginKey);
    s.setValue("rootPath", m_rootPath);
    s.setValue("outputMode", m_outputMode);
    s.setValue("outputDir", m_outputDir);
    s.setValue("recursive", m_recursive);
    s.setValue("resizeMode", m_resizeMode);
    s.setValue("resizeRatio", m_resizeRatio);
    s.setValue("resizeWidth", m_resizeWidth);
    s.setValue("resizeHeight", m_resizeHeight);
    s.setValue("mode", m_mode);
    s.sync();
}
