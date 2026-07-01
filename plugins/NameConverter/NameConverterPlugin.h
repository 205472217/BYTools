#pragma once

#include "../../src/core/PluginInterface.h"
#include "NameConverterController.h"

class NameConverterSettings;
class PluginLogger;

class NameConverterPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    static constexpr const char* kIniSection = "name-converter";

    NameConverterPlugin(QObject *parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString iconName() const override;
    QString category() const override;
    int order() const override;

    void initialize() override;
    void cleanup() override;

    QObject* getController() override;
    QObject* getSettings() override;

private:
    PluginLogger *m_logger = nullptr;
    NameConverterController *m_controller;
    NameConverterSettings *m_settings = nullptr;
};