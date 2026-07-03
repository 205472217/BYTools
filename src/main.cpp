#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QDebug>

#include "core/PluginManager.h"
#include "core/PluginInterface.h"
#include "core/ThemeManager.h"
#include "core/MpvPlayer.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Fusion");

    qDebug() << "=== Starting BYTools ===";
    qDebug() << "Application dir:" << QCoreApplication::applicationDirPath();

    QStringList loadedPlugins = PluginManager::instance()->loadPlugins();
    qDebug() << "Loaded plugins count:" << loadedPlugins.count();
    qDebug() << "Loaded plugins:" << loadedPlugins;

    // 启动时异步解压 mpv.zip，不阻塞界面
    for (const QString &mpvId : PluginManager::instance()->mpvPluginIds())
        PluginManager::instance()->startMpvExtraction(mpvId);

    QQmlApplicationEngine engine;
    qmlRegisterType<MpvPlayer>("BYTools", 1, 0, "MpvPlayer");

    engine.rootContext()->setContextProperty("pluginManager", PluginManager::instance());
    engine.rootContext()->setContextProperty("themeManager", ThemeManager::instance());


    QObject::connect(
        &engine,
        &QQmlEngine::warnings,
        [](const QList<QQmlError> &warnings) {
            for (const auto &w : warnings)
                qDebug() << "QML:" << w.toString();
        });

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("BYTools", "Main");
    return app.exec();
}
