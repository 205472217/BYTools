#include "SettingsHelper.h"
#include "Config.h"

#include <QMutex>
#include <QMutexLocker>
#include <QHash>

static QHash<QString, QSettings *> &settingsInstances()
{
    static QHash<QString, QSettings *> instances;
    return instances;
}

static void cleanupSettings()
{
    qDeleteAll(settingsInstances());
    settingsInstances().clear();
}

QSettings &pluginGroupSettings(const char *groupName)
{
    static QMutex mutex;
    static bool cleanupRegistered = false;

    QMutexLocker lock(&mutex);
    if (!cleanupRegistered) {
        cleanupRegistered = true;
        qAddPostRoutine(cleanupSettings);
    }

    QString key = QLatin1String(groupName);
    auto &instances = settingsInstances();
    auto it = instances.constFind(key);
    if (it != instances.constEnd())
        return **it;

    auto *s = new QSettings(pluginConfigFilePath(), QSettings::IniFormat);
    s->beginGroup(key);
    instances.insert(key, s);
    return *s;
}
