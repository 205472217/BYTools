#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QMap>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>

class PluginInterface;

class PluginManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY pluginsChanged)
    Q_PROPERTY(QStringList pluginCategories READ pluginCategories NOTIFY pluginsChanged)

public:
    static PluginManager* instance();

    QVariantList plugins() const;
    QStringList pluginCategories() const;
    QStringList loadPlugins(const QString &pluginPath = QString());
    Q_INVOKABLE QObject* getPlugin(const QString &id);
    Q_INVOKABLE QObject* getPluginSettings(const QString &id);
    Q_INVOKABLE QString pluginDirectory(const QString &id) const;
    Q_INVOKABLE bool fileExists(const QString &filePath) const;
    Q_INVOKABLE bool extractMpvZip(const QString &pluginId);
    void registerPlugin(PluginInterface *plugin);

signals:
    void pluginsChanged();

private:
    PluginManager(QObject *parent = nullptr);
    ~PluginManager();

    struct PluginEntry {
        PluginInterface *plugin = nullptr;
        QPluginLoader *loader = nullptr;
    };

    QMap<QString, PluginEntry> m_plugins;
    QStringList m_loadedPluginPaths;

    void loadPluginsFromDir(const QDir &dir, const QStringList &filters, QStringList &loadedPlugins);
};