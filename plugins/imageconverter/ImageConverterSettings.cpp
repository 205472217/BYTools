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

void ImageConverterSettings::setRootPath(const QString &path)
{
    if (m_rootPath != path) {
        m_rootPath = path;
        emit rootPathChanged();
        saveSettings();
    }
}
void ImageConverterSettings::setTargetFormat(int format)
{
    if (m_targetFormat != format) {
        m_targetFormat = format;
        emit targetFormatChanged();
    }
}
void ImageConverterSettings::setQuality(int quality)
{
    if (m_quality != quality) {
        m_quality = quality;
        emit qualityChanged();
    }
}
void ImageConverterSettings::setBgColor(const QString &color)
{
    if (m_bgColor != color) {
        m_bgColor = color;
        emit bgColorChanged();
    }
}
void ImageConverterSettings::setOutputMode(int mode)
{
    if (m_outputMode != mode) {
        m_outputMode = mode;
        emit outputModeChanged();
        saveSettings();
    }
}
void ImageConverterSettings::setOutputDir(const QString &dir)
{
    if (m_outputDir != dir) {
        m_outputDir = dir;
        emit outputDirChanged();
        saveSettings();
    }
}
void ImageConverterSettings::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        emit recursiveChanged();
        saveSettings();
    }
}

void ImageConverterSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings(ImageConverterPlugin::kIniSection);
    m_rootPath = s.value("rootPath").toString();
    m_outputMode = s.value("outputMode", 0).toInt();
    m_outputDir = s.value("outputDir").toString();
    m_recursive = s.value("recursive", false).toBool();
}
void ImageConverterSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings(ImageConverterPlugin::kIniSection);
    s.setValue("rootPath", m_rootPath);
    s.setValue("outputMode", m_outputMode);
    s.setValue("outputDir", m_outputDir);
    s.setValue("recursive", m_recursive);
    s.sync();
}
