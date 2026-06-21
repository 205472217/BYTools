#include "SubtitleAdjustSettings.h"
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

void SubtitleAdjustSettings::setMode(int mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        emit modeChanged();
        saveSettings();
    }
}
void SubtitleAdjustSettings::setVideoFolder(const QString &path)
{
    if (m_videoFolder != path) {
        m_videoFolder = path;
        emit videoFolderChanged();
        saveSettings();
    }
}
void SubtitleAdjustSettings::setSubtitleFolder(const QString &path)
{
    if (m_subtitleFolder != path) {
        m_subtitleFolder = path;
        emit subtitleFolderChanged();
        saveSettings();
    }
}
void SubtitleAdjustSettings::setRecursiveVideo(bool recursive)
{
    if (m_recursiveVideo != recursive) {
        m_recursiveVideo = recursive;
        emit recursiveVideoChanged();
        saveSettings();
    }
}
void SubtitleAdjustSettings::setRecursiveSubtitle(bool recursive)
{
    if (m_recursiveSubtitle != recursive) {
        m_recursiveSubtitle = recursive;
        emit recursiveSubtitleChanged();
        saveSettings();
    }
}
void SubtitleAdjustSettings::setOverwriteOriginal(bool overwrite)
{
    if (m_overwriteOriginal != overwrite) {
        m_overwriteOriginal = overwrite;
        emit overwriteOriginalChanged();
        saveSettings();
    }
}

void SubtitleAdjustSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings("subtitle-adjust");
    m_mode = s.value("mode", 0).toInt();
    m_videoFolder = s.value("videoFolder").toString();
    m_subtitleFolder = s.value("subtitleFolder").toString();
    m_recursiveVideo = s.value("recursiveVideo", false).toBool();
    m_recursiveSubtitle = s.value("recursiveSubtitle", false).toBool();
    m_overwriteOriginal = s.value("overwriteOriginal", false).toBool();
}
void SubtitleAdjustSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings("subtitle-adjust");
    s.setValue("mode", m_mode);
    s.setValue("videoFolder", m_videoFolder);
    s.setValue("subtitleFolder", m_subtitleFolder);
    s.setValue("recursiveVideo", m_recursiveVideo);
    s.setValue("recursiveSubtitle", m_recursiveSubtitle);
    s.setValue("overwriteOriginal", m_overwriteOriginal);
    s.sync();
}
