#include "AppController.h"
#include "../core/PluginManager.h"

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

QVariantList AppController::features() const
{
    return PluginManager::instance()->plugins();
}