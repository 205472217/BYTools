#include "AppController.h"

#include <QVariantMap>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    registerBuiltInFeatures();
}

QVariantList AppController::features() const
{
    QVariantList result;
    for (const auto &feature : m_features) {
        QVariantMap item;
        item["id"] = feature.id;
        item["title"] = feature.title;
        item["description"] = feature.description;
        item["iconName"] = feature.iconName;
        result.append(item);
    }
    return result;
}

void AppController::registerBuiltInFeatures()
{
    m_features.append({
        "rename-converter",
        QStringLiteral("文件名繁转简"),
        QStringLiteral("批量转换文件、文件夹名称"),
        "translate"
    });
}
