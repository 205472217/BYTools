#pragma once

#include "../../src/core/PluginInterface.h"

class CustomSubtitleController;

class CustomSubtitlePlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    explicit CustomSubtitlePlugin(QObject *parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString iconName() const override;

    void initialize() override;
    void cleanup() override;

    QObject* getController() override;

private:
    CustomSubtitleController *m_controller = nullptr;
};
