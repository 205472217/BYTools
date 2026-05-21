#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QMap>

class PluginInterface;

class PluginManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY pluginsChanged)

public:
    static PluginManager* instance();

    QVariantList plugins() const;
    QStringList loadPlugins(const QString &pluginPath = QString());
    Q_INVOKABLE QObject* getPlugin(const QString &id);
    void registerPlugin(PluginInterface *plugin);

signals:
    void pluginsChanged();

private:
    PluginManager(QObject *parent = nullptr);
    ~PluginManager();

    QMap<QString, PluginInterface*> m_plugins;
    QStringList m_loadedPluginPaths;
};