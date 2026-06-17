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

QStringList AppController::pluginCategories() const
{
    return PluginManager::instance()->pluginCategories();
}