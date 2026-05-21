#include "PluginManager.h"
#include "PluginInterface.h"

#include <QDir>
#include <QPluginLoader>
#include <QVariantMap>
#include <QCoreApplication>

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
}

PluginManager::~PluginManager()
{
    for (auto plugin : m_plugins.values()) {
        plugin->cleanup();
    }
}

PluginManager* PluginManager::instance()
{
    static PluginManager manager;
    return &manager;
}

QVariantList PluginManager::plugins() const
{
    QVariantList result;
    for (auto plugin : m_plugins.values()) {
        QVariantMap item;
        item["id"] = plugin->id();
        item["name"] = plugin->name();
        item["description"] = plugin->description();
        item["iconName"] = plugin->iconName();
        result.append(item);
    }
    return result;
}

QStringList PluginManager::loadPlugins(const QString &pluginPath)
{
    QStringList loadedPlugins;
    QString path = pluginPath.isEmpty() ? QCoreApplication::applicationDirPath() + "/plugins" : pluginPath;
    QDir pluginDir(path);

    if (!pluginDir.exists()) {
        return loadedPlugins;
    }

    QStringList filters = {"*.dll", "*.so", "*.dylib"};
    QFileInfoList files = pluginDir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &file : files) {
        QPluginLoader loader(file.absoluteFilePath());
        QObject *instance = loader.instance();

        if (instance) {
            PluginInterface *plugin = qobject_cast<PluginInterface*>(instance);
            if (plugin) {
                plugin->initialize();
                m_plugins[plugin->id()] = plugin;
                m_loadedPluginPaths.append(file.absoluteFilePath());
                loadedPlugins.append(plugin->id());
            }
        }
    }

    return loadedPlugins;
}

QObject* PluginManager::getPlugin(const QString &id)
{
    PluginInterface* plugin = m_plugins.value(id, nullptr);
    return plugin ? plugin->getController() : nullptr;
}

void PluginManager::registerPlugin(PluginInterface *plugin)
{
    if (plugin && !m_plugins.contains(plugin->id())) {
        plugin->initialize();
        m_plugins[plugin->id()] = plugin;
        emit pluginsChanged();
    }
}