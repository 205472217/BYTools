#include "CustomSubtitleSettings.h"
#include "CustomSubtitlePlugin.h"
#include "SettingsHelper.h"
#include <QDir>

CustomSubtitleSettings::CustomSubtitleSettings(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

QString CustomSubtitleSettings::subtitleDownloadPath() const { return m_subtitleDownloadPath; }
QString CustomSubtitleSettings::videoSourcePath() const { return m_videoSourcePath; }
bool CustomSubtitleSettings::recursive() const { return m_recursive; }
QString CustomSubtitleSettings::mergedOutputPath() const { return m_mergedOutputPath; }
QString CustomSubtitleSettings::ffmpegPath() const { return m_ffmpegPath; }
bool CustomSubtitleSettings::gpuAccel() const { return m_gpuAccel; }
bool CustomSubtitleSettings::weakMatch() const { return m_weakMatch; }
QStringList CustomSubtitleSettings::enabledPreprocessors() const { return m_enabledPreprocessors; }

void CustomSubtitleSettings::setSubtitleDownloadPath(const QString &path)
{
    if (m_subtitleDownloadPath != path) {
        m_subtitleDownloadPath = path;
        saveSettings();
    }
}
void CustomSubtitleSettings::setVideoSourcePath(const QString &path)
{
    if (m_videoSourcePath != path) {
        m_videoSourcePath = path;
        saveSettings();
    }
}
void CustomSubtitleSettings::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        saveSettings();
    }
}
void CustomSubtitleSettings::setMergedOutputPath(const QString &path)
{
    if (m_mergedOutputPath != path) {
        m_mergedOutputPath = path;
        saveSettings();
    }
}
void CustomSubtitleSettings::setFfmpegPath(const QString &path)
{
    if (m_ffmpegPath != path) {
        m_ffmpegPath = path;
        saveSettings();
    }
}
void CustomSubtitleSettings::setGpuAccel(bool enable)
{
    if (m_gpuAccel != enable) {
        m_gpuAccel = enable;
        saveSettings();
    }
}
void CustomSubtitleSettings::setWeakMatch(bool weak)
{
    if (m_weakMatch != weak) {
        m_weakMatch = weak;
        saveSettings();
    }
}
void CustomSubtitleSettings::setEnabledPreprocessors(const QStringList &ops)
{
    if (m_enabledPreprocessors != ops) {
        m_enabledPreprocessors = ops;
        saveSettings();
    }
}

void CustomSubtitleSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings(CustomSubtitlePlugin::PluginKey);
    m_subtitleDownloadPath = s.value("customSubtitleDownloadPath").toString();
    m_videoSourcePath = s.value("customVideoSourcePath").toString();
    m_recursive = s.value("customRecursive", false).toBool();
    m_mergedOutputPath = s.value("customMergedOutputPath").toString();
    m_ffmpegPath = s.value("customFfmpegPath").toString();
    m_gpuAccel = s.value("customGpuAccel", false).toBool();
    m_weakMatch = s.value("customWeakMatch", false).toBool();
    m_enabledPreprocessors = s.value("customPreprocessors").toStringList();
}
void CustomSubtitleSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings(CustomSubtitlePlugin::PluginKey);
    s.setValue("customSubtitleDownloadPath", m_subtitleDownloadPath);
    s.setValue("customVideoSourcePath", m_videoSourcePath);
    s.setValue("customRecursive", m_recursive);
    s.setValue("customMergedOutputPath", m_mergedOutputPath);
    s.setValue("customFfmpegPath", m_ffmpegPath);
    s.setValue("customGpuAccel", m_gpuAccel);
    s.setValue("customWeakMatch", m_weakMatch);
    s.setValue("customPreprocessors", m_enabledPreprocessors);
    s.sync();
}
