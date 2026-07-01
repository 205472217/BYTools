#pragma once

#include "../../src/core/PluginInterface.h"
#include "SubtitleAdjustController.h"

class PluginLogger;
class SubtitleAdjustSettings;

class SubtitleAdjustPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    SubtitleAdjustPlugin(QObject *parent = nullptr);

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
    bool needsMpv() const override;

private:
    PluginLogger *m_logger = nullptr;
    SubtitleAdjustController *m_controller = nullptr;
    SubtitleAdjustSettings *m_settings = nullptr;
};
