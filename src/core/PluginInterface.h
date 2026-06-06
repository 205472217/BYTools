#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class PluginInterface
{
public:
    virtual ~PluginInterface() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QString iconName() const = 0;

    virtual void initialize() = 0;
    virtual void cleanup() = 0;

    virtual QObject* getController() = 0;
    virtual QObject* getSettings() { return nullptr; }
};

#define PluginInterface_iid "com.bytools.PluginInterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)