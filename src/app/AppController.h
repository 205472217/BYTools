#pragma once

#include <QObject>
#include <QVariantList>

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList features READ features CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);

    QVariantList features() const;
};