#include "FileViewSettings.h"
#include "SettingsHelper.h"

FileViewSettings::FileViewSettings(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

QString FileViewSettings::sourceFolder() const { return m_sourceFolder; }
bool FileViewSettings::recursive() const { return m_recursive; }
int FileViewSettings::fileType() const { return m_fileType; }
int FileViewSettings::sortField() const { return m_sortField; }
bool FileViewSettings::sortAscending() const { return m_sortAscending; }
int FileViewSettings::volume() const { return m_volume; }
bool FileViewSettings::muted() const { return m_muted; }
int FileViewSettings::seekStepMs() const { return m_seekStepMs; }

void FileViewSettings::setSourceFolder(const QString &path)
{
    if (m_sourceFolder != path) {
        m_sourceFolder = path;
        emit sourceFolderChanged();
        saveSettings();
    }
}
void FileViewSettings::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        emit recursiveChanged();
        saveSettings();
    }
}
void FileViewSettings::setFileType(int type)
{
    if (m_fileType != type) {
        m_fileType = type;
        emit fileTypeChanged();
        saveSettings();
    }
}
void FileViewSettings::setSortField(int field)
{
    if (m_sortField != field) {
        m_sortField = field;
        emit sortFieldChanged();
        saveSettings();
    }
}
void FileViewSettings::setSortAscending(bool ascending)
{
    if (m_sortAscending != ascending) {
        m_sortAscending = ascending;
        emit sortAscendingChanged();
        saveSettings();
    }
}
void FileViewSettings::setVolume(int vol)
{
    vol = qBound(0, vol, 100);
    if (m_volume == vol)
        return;
    m_volume = vol;
    emit volumeChanged();
    saveSettings();
}
void FileViewSettings::setMuted(bool m)
{
    if (m_muted == m)
        return;
    m_muted = m;
    emit mutedChanged();
    saveSettings();
}
void FileViewSettings::setSeekStepMs(int ms)
{
    if (m_seekStepMs == ms)
        return;
    m_seekStepMs = ms;
    emit seekStepMsChanged();
    saveSettings();
}

void FileViewSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings("FileView");
    m_sourceFolder = s.value("sourceFolder").toString();
    m_recursive = s.value("recursive", false).toBool();
    m_fileType = s.value("fileType", 0).toInt();
    m_sortField = s.value("sortField", 0).toInt();
    m_sortAscending = s.value("sortAscending", true).toBool();
    double v = s.value("volume", 100.0).toDouble();
    m_volume = qBound(0, qRound(v), 100);
    m_muted = s.value("muted", false).toBool();
    m_seekStepMs = s.value("seekStepMs", 5000).toInt();
}
void FileViewSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings("FileView");
    s.setValue("sourceFolder", m_sourceFolder);
    s.setValue("recursive", m_recursive);
    s.setValue("fileType", m_fileType);
    s.setValue("sortField", m_sortField);
    s.setValue("sortAscending", m_sortAscending);
    s.setValue("volume", m_volume);
    s.setValue("muted", m_muted);
    s.setValue("seekStepMs", m_seekStepMs);
    s.sync();
}
