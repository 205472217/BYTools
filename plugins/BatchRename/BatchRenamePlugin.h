#pragma once

#include "../../src/core/PluginInterface.h"

class BatchRenameController;
class BatchRenameSettings;
class PluginLogger;

class BatchRenamePlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    static constexpr const char* kIniSection = "batch-rename";

    BatchRenamePlugin(QObject *parent = nullptr);

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
    BatchRenameController *m_controller;
    BatchRenameSettings *m_settings = nullptr;
};