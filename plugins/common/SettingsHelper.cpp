#include "SettingsHelper.h"
#include "Config.h"

#include <QMutex>
#include <QMutexLocker>
#include <QHash>

QSettings &pluginGroupSettings(const char *groupName)
{
    static QMutex mutex;
    static QHash<QString, QSettings *> instances;

    QMutexLocker lock(&mutex);
    QString key = QLatin1String(groupName);
    auto it = instances.constFind(key);
    if (it != instances.constEnd())
        return **it;

    auto *s = new QSettings(pluginConfigFilePath(), QSettings::IniFormat);
    s->beginGroup(key);
    instances.insert(key, s);
    return *s;
}
