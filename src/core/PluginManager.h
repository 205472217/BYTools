#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QMap>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QProcess>

class PluginInterface;

class PluginManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY pluginsChanged)
    Q_PROPERTY(QStringList pluginCategories READ pluginCategories NOTIFY pluginsChanged)
    Q_PROPERTY(bool mpvExtracting READ isMpvExtracting NOTIFY mpvExtractingChanged)

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
    Q_INVOKABLE void startMpvExtraction(const QString &pluginId);
    Q_INVOKABLE QStringList allPluginIds() const;
    Q_INVOKABLE QString pluginQmlUrl(const QString &id, const QString &pageType = "page") const;
    Q_INVOKABLE QStringList mpvPluginIds() const;
    Q_INVOKABLE bool hasProcessingTasks() const;
    bool isMpvExtracting() const;
    void registerPlugin(PluginInterface *plugin);

signals:
    void pluginsChanged();
    void mpvExtractingChanged();

private:
    PluginManager(QObject *parent = nullptr);
    ~PluginManager();

    struct PluginEntry {
        PluginInterface *plugin = nullptr;
        QPluginLoader *loader = nullptr;
    };

    QMap<QString, PluginEntry> m_plugins;
    QMap<QString, QString> m_pageFiles;
    QMap<QString, QString> m_settingsFiles;
    QStringList m_loadedPluginPaths;
    int m_mpvExtractionCount = 0;

    void registerPluginQmlMaps(const QString &id);
    void loadPluginsFromDir(const QDir &dir, const QStringList &filters, QStringList &loadedPlugins);
};