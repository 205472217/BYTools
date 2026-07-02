#include "SubtitleAdjustSettings.h"
#include "SubtitleAdjustPlugin.h"
#include "SettingsHelper.h"

SubtitleAdjustSettings::SubtitleAdjustSettings(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

int SubtitleAdjustSettings::mode() const { return m_mode; }
QString SubtitleAdjustSettings::videoFolder() const { return m_videoFolder; }
QString SubtitleAdjustSettings::subtitleFolder() const { return m_subtitleFolder; }
bool SubtitleAdjustSettings::recursiveVideo() const { return m_recursiveVideo; }
bool SubtitleAdjustSettings::recursiveSubtitle() const { return m_recursiveSubtitle; }
bool SubtitleAdjustSettings::overwriteOriginal() const { return m_overwriteOriginal; }
int SubtitleAdjustSettings::volume() const { return m_volume; }
bool SubtitleAdjustSettings::muted() const { return m_muted; }
int SubtitleAdjustSettings::seekStepMs() const { return m_seekStepMs; }

void SubtitleAdjustSettings::setMode(int mode)
{
    if (m_mode != mode) {
        m_mode = mode;

        saveSettings();
    }
}
void SubtitleAdjustSettings::setVideoFolder(const QString &path)
{
    if (m_videoFolder != path) {
        m_videoFolder = path;

        saveSettings();
    }
}
void SubtitleAdjustSettings::setSubtitleFolder(const QString &path)
{
    if (m_subtitleFolder != path) {
        m_subtitleFolder = path;

        saveSettings();
    }
}
void SubtitleAdjustSettings::setRecursiveVideo(bool recursive)
{
    if (m_recursiveVideo != recursive) {
        m_recursiveVideo = recursive;

        saveSettings();
    }
}
void SubtitleAdjustSettings::setRecursiveSubtitle(bool recursive)
{
    if (m_recursiveSubtitle != recursive) {
        m_recursiveSubtitle = recursive;

        saveSettings();
    }
}
void SubtitleAdjustSettings::setOverwriteOriginal(bool overwrite)
{
    if (m_overwriteOriginal != overwrite) {
        m_overwriteOriginal = overwrite;

        saveSettings();
    }
}
void SubtitleAdjustSettings::setVolume(int vol)
{
    vol = qBound(0, vol, 100);
    if (m_volume == vol)
        return;
    m_volume = vol;

    saveSettings();
}
void SubtitleAdjustSettings::setMuted(bool m)
{
    if (m_muted == m)
        return;
    m_muted = m;

    saveSettings();
}
void SubtitleAdjustSettings::setSeekStepMs(int ms)
{
    if (m_seekStepMs == ms)
        return;
    m_seekStepMs = ms;

    saveSettings();
}

void SubtitleAdjustSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings(SubtitleAdjustPlugin::PluginKey);
    m_mode = s.value("mode", 0).toInt();
    m_videoFolder = s.value("videoFolder").toString();
    m_subtitleFolder = s.value("subtitleFolder").toString();
    m_recursiveVideo = s.value("recursiveVideo", false).toBool();
    m_recursiveSubtitle = s.value("recursiveSubtitle", false).toBool();
    m_overwriteOriginal = s.value("overwriteOriginal", false).toBool();
    m_volume = qBound(0, s.value("volume", 100).toInt(), 100);
    m_muted = s.value("muted", false).toBool();
    m_seekStepMs = s.value("seekStepMs", 5000).toInt();
}
void SubtitleAdjustSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings(SubtitleAdjustPlugin::PluginKey);
    s.setValue("mode", m_mode);
    s.setValue("videoFolder", m_videoFolder);
    s.setValue("subtitleFolder", m_subtitleFolder);
    s.setValue("recursiveVideo", m_recursiveVideo);
    s.setValue("recursiveSubtitle", m_recursiveSubtitle);
    s.setValue("overwriteOriginal", m_overwriteOriginal);
    s.setValue("volume", m_volume);
    s.setValue("muted", m_muted);
    s.setValue("seekStepMs", m_seekStepMs);
    s.sync();
}
