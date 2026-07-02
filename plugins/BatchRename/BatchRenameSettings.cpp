#include "BatchRenameSettings.h"
#include "BatchRenamePlugin.h"
#include "SettingsHelper.h"
#include <QDir>

BatchRenameSettings::BatchRenameSettings(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

QString BatchRenameSettings::rootPath() const { return m_rootPath; }
int BatchRenameSettings::fileType() const { return m_fileType; }
QString BatchRenameSettings::customExtension() const { return m_customExtension; }
int BatchRenameSettings::renameMode() const { return m_renameMode; }
QString BatchRenameSettings::baseName() const { return m_baseName; }
QString BatchRenameSettings::searchText() const { return m_searchText; }
QString BatchRenameSettings::replaceText() const { return m_replaceText; }
bool BatchRenameSettings::recursive() const { return m_recursive; }

void BatchRenameSettings::setRootPath(const QString &path)
{
    if (m_rootPath != path) {
        m_rootPath = path;
        saveSettings();
    }
}
void BatchRenameSettings::setFileType(int fileType)
{
    if (m_fileType != fileType) {
        m_fileType = fileType;
    }
}
void BatchRenameSettings::setCustomExtension(const QString &ext)
{
    if (m_customExtension != ext) {
        m_customExtension = ext;
    }
}
void BatchRenameSettings::setRenameMode(int mode)
{
    if (m_renameMode != mode) {
        m_renameMode = mode;
    }
}
void BatchRenameSettings::setBaseName(const QString &name)
{
    if (m_baseName != name) {
        m_baseName = name;
    }
}
void BatchRenameSettings::setSearchText(const QString &text)
{
    if (m_searchText != text) {
        m_searchText = text;
    }
}
void BatchRenameSettings::setReplaceText(const QString &text)
{
    if (m_replaceText != text) {
        m_replaceText = text;
    }
}
void BatchRenameSettings::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        saveSettings();
    }
}

void BatchRenameSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings(BatchRenamePlugin::PluginKey);
    m_rootPath = s.value("rootPath").toString();
    m_recursive = s.value("recursive", false).toBool();
}
void BatchRenameSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings(BatchRenamePlugin::PluginKey);
    s.setValue("rootPath", m_rootPath);
    s.setValue("recursive", m_recursive);
    s.sync();
}
