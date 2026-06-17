#pragma once

#include <QObject>
#include <QVariantList>

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList features READ features CONSTANT)
    Q_PROPERTY(QStringList pluginCategories READ pluginCategories CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);

    QVariantList features() const;
    QStringList pluginCategories() const;
};