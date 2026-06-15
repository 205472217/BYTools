#include "PluginManager.h"
#include "PluginInterface.h"

#include <QDir>
#include <QPluginLoader>
#include <QVariantMap>
#include <QCoreApplication>
#include <QDebug>
#include <algorithm>

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
    QList<PluginInterface*> sorted = m_plugins.values();
    std::sort(sorted.begin(), sorted.end(), [](PluginInterface *a, PluginInterface *b) {
        return a->order() < b->order();
    });

    QVariantList result;
    for (auto plugin : sorted) {
        QVariantMap item;
        item["id"] = plugin->id();
        item["name"] = plugin->name();
        item["description"] = plugin->description();
        item["iconName"] = plugin->iconName();
        result.append(item);
    }
    return result;
}

void PluginManager::loadPluginsFromDir(const QDir &dir, const QStringList &filters, QStringList &loadedPlugins)
{
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &file : files) {
        QPluginLoader loader(file.absoluteFilePath());
        QObject *instance = loader.instance();

        if (!instance) {
            qWarning() << "✗ 插件加载失败:" << file.fileName() << "-" << loader.errorString();
            continue;
        }

        PluginInterface *plugin = qobject_cast<PluginInterface*>(instance);
        if (!plugin) {
            qWarning() << "✗ 插件接口不兼容:" << file.fileName();
            delete instance;
            continue;
        }

        QString pluginId = plugin->id();
        if (m_plugins.contains(pluginId)) {
            qWarning() << "⚠ 插件 ID 重复，跳过:" << pluginId << file.fileName();
            continue;
        }

        plugin->initialize();
        m_plugins[pluginId] = plugin;
        m_loadedPluginPaths.append(file.absoluteFilePath());
        loadedPlugins.append(pluginId);
        qDebug() << "✓ 插件已加载:" << pluginId << file.fileName();
    }

    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subDirInfo : subDirs) {
        QDir subDir(subDirInfo.absoluteFilePath());
        loadPluginsFromDir(subDir, filters, loadedPlugins);
    }
}

QStringList PluginManager::loadPlugins(const QString &pluginPath)
{
    QStringList loadedPlugins;
    QString path = pluginPath.isEmpty() ? QCoreApplication::applicationDirPath() + "/plugins" : pluginPath;
    QDir pluginDir(path);

    if (!pluginDir.exists()) {
        qWarning() << "✗ 插件目录不存在:" << path;
        return loadedPlugins;
    }
    qDebug() << "加载插件目录:" << path;

    QStringList filters = {"*.dll", "*.so", "*.dylib"};

    loadPluginsFromDir(pluginDir, filters, loadedPlugins);

    return loadedPlugins;
}

QObject* PluginManager::getPlugin(const QString &id)
{
    PluginInterface* plugin = m_plugins.value(id, nullptr);
    return plugin ? plugin->getController() : nullptr;
}

QObject* PluginManager::getPluginSettings(const QString &id)
{
    PluginInterface* plugin = m_plugins.value(id, nullptr);
    return plugin ? plugin->getSettings() : nullptr;
}

void PluginManager::registerPlugin(PluginInterface *plugin)
{
    if (plugin && !m_plugins.contains(plugin->id())) {
        plugin->initialize();
        m_plugins[plugin->id()] = plugin;
        emit pluginsChanged();
    }
}