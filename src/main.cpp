#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "app/AppController.h"
#include "features/renameconverter/BatchRenameController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    AppController appController;
    BatchRenameController batchRenameController;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &appController);
    engine.rootContext()->setContextProperty("batchRenameController", &batchRenameController);
    engine.rootContext()->setContextProperty("renamePreviewModel", batchRenameController.previewModel());

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("BYTools", "Main");
    return app.exec();
}
