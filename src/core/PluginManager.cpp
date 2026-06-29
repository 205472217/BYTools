#include "PluginManager.h"
#include "PluginInterface.h"

#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QVariantMap>
#include <QSet>
#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <algorithm>

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
}

PluginManager::~PluginManager()
{
    // 终止所有运行中的解压进程
    const auto processes = findChildren<QProcess*>();
    for (auto *p : processes) {
        if (p->state() != QProcess::NotRunning) {
            p->kill();
            p->waitForFinished(3000);
        }
    }

    for (auto &entry : m_plugins) {
        if (entry.plugin)
            entry.plugin->cleanup();
        delete entry.loader;
    }
    m_plugins.clear();
}

PluginManager* PluginManager::instance()
{
    static PluginManager manager;
    return &manager;
}

QVariantList PluginManager::plugins() const
{
    QList<PluginInterface*> sorted;
    for (const auto &entry : m_plugins) {
        if (entry.plugin)
            sorted.append(entry.plugin);
    }
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
        item["category"] = plugin->category();
        result.append(item);
    }
    return result;
}

QStringList PluginManager::pluginCategories() const
{
    QSet<QString> categories;
    for (const auto &entry : m_plugins) {
        if (entry.plugin)
            categories.insert(entry.plugin->category());
    }
    QStringList sorted = categories.values();
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

void PluginManager::loadPluginsFromDir(const QDir &dir, const QStringList &filters, QStringList &loadedPlugins)
{
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &file : files) {
        QPluginLoader *loader = new QPluginLoader(file.absoluteFilePath());
        QObject *instance = loader->instance();

        if (!instance) {
            qWarning() << "✗ 插件加载失败:" << file.fileName() << "-" << loader->errorString();
            delete loader;
            continue;
        }

        PluginInterface *plugin = qobject_cast<PluginInterface*>(instance);
        if (!plugin) {
            qWarning() << "✗ 插件接口不兼容:" << file.fileName();
            delete instance;
            delete loader;
            continue;
        }

        QString pluginId = plugin->id();
        if (m_plugins.contains(pluginId)) {
            qWarning() << "⚠ 插件 ID 重复，跳过:" << pluginId << file.fileName();
            delete loader;
            continue;
        }

        plugin->initialize();
        m_plugins[pluginId] = {plugin, loader};
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
    auto it = m_plugins.constFind(id);
    if (it == m_plugins.constEnd() || !it->plugin)
        return nullptr;
    return it->plugin->getController();
}

QObject* PluginManager::getPluginSettings(const QString &id)
{
    auto it = m_plugins.constFind(id);
    if (it == m_plugins.constEnd() || !it->plugin)
        return nullptr;
    return it->plugin->getSettings();
}

bool PluginManager::fileExists(const QString &filePath) const
{
    return QFileInfo::exists(filePath);
}

QString PluginManager::pluginDirectory(const QString &id) const
{
    auto it = m_plugins.constFind(id);
    if (it == m_plugins.constEnd() || !it->loader)
        return {};
    return QFileInfo(it->loader->fileName()).absolutePath();
}

bool PluginManager::extractMpvZip(const QString &pluginId)
{
    auto it = m_plugins.constFind(pluginId);
    if (it == m_plugins.constEnd() || !it->loader)
        return false;

    QString pluginDir = QFileInfo(it->loader->fileName()).absolutePath();
    QString mpvExe = pluginDir + "/mpv/mpv.exe";

    if (QFileInfo::exists(mpvExe))
        return true;

    QString mpvZip = pluginDir + "/mpv/mpv.zip";
    if (!QFileInfo::exists(mpvZip))
        return false;

    QProcess proc;
    proc.start("powershell", QStringList{}
        << "-NoProfile"
        << "-Command"
        << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
            .arg(mpvZip, pluginDir + "/mpv"));
    proc.waitForFinished(60000);

    return QFileInfo::exists(mpvExe);
}

void PluginManager::startMpvExtraction(const QString &pluginId)
{
    auto it = m_plugins.constFind(pluginId);
    if (it == m_plugins.constEnd() || !it->loader)
        return;

    QString pluginDir = QFileInfo(it->loader->fileName()).absolutePath();
    QString mpvExe = pluginDir + "/mpv/mpv.exe";

    if (QFileInfo::exists(mpvExe))
        return;

    QString mpvZip = pluginDir + "/mpv/mpv.zip";
    if (!QFileInfo::exists(mpvZip))
        return;

    bool wasExtracting = (m_mpvExtractionCount > 0);
    m_mpvExtractionCount++;
    if (!wasExtracting)
        emit mpvExtractingChanged();

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc]() {
        m_mpvExtractionCount--;
        if (m_mpvExtractionCount < 0)
            m_mpvExtractionCount = 0;
        if (m_mpvExtractionCount == 0)
            emit mpvExtractingChanged();
        proc->deleteLater();
    });

    proc->start("powershell", QStringList{}
        << "-NoProfile"
        << "-Command"
        << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
            .arg(mpvZip, pluginDir + "/mpv"));
}

bool PluginManager::isMpvExtracting() const
{
    return m_mpvExtractionCount > 0;
}

void PluginManager::registerPlugin(PluginInterface *plugin)
{
    if (plugin && !m_plugins.contains(plugin->id())) {
        plugin->initialize();
        m_plugins[plugin->id()] = {plugin, nullptr};
        emit pluginsChanged();
    }
}