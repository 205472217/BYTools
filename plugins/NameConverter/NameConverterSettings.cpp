#include "NameConverterSettings.h"
#include "NameConverterPlugin.h"
#include "SettingsHelper.h"
#include <QDir>

NameConverterSettings::NameConverterSettings(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

QString NameConverterSettings::rootPath() const { return m_rootPath; }
int NameConverterSettings::targetType() const { return m_targetType; }
bool NameConverterSettings::recursive() const { return m_recursive; }

void NameConverterSettings::setRootPath(const QString &path)
{
    if (m_rootPath != path) {
        m_rootPath = path;
        emit rootPathChanged();
        saveSettings();
    }
}
void NameConverterSettings::setTargetType(int type)
{
    if (m_targetType != type) {
        m_targetType = type;
        emit targetTypeChanged();
        saveSettings();
    }
}
void NameConverterSettings::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        emit recursiveChanged();
        saveSettings();
    }
}

void NameConverterSettings::loadSettings()
{
    QSettings &s = pluginGroupSettings(NameConverterPlugin::kIniSection);
    m_rootPath = s.value("rootPath").toString();
    m_targetType = s.value("targetType", 2).toInt();
    m_recursive = s.value("recursive", false).toBool();
}
void NameConverterSettings::saveSettings()
{
    QSettings &s = pluginGroupSettings(NameConverterPlugin::kIniSection);
    s.setValue("rootPath", m_rootPath);
    s.setValue("targetType", m_targetType);
    s.setValue("recursive", m_recursive);
    s.sync();
}
