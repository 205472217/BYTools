#pragma once

#include "../../src/core/PluginInterface.h"
#include "BatchNameController.h"

class PluginLogger;

class NameConverterPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    NameConverterPlugin(QObject *parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString iconName() const override;

    void initialize() override;
    void cleanup() override;

    QObject* getController() override;

private:
    PluginLogger *m_logger = nullptr;
    BatchNameController *m_controller;
};