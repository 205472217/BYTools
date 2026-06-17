#pragma once

#include "../../src/core/PluginInterface.h"

class ImageCropController;
class PluginLogger;

class ImageCropPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    ImageCropPlugin(QObject *parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString iconName() const override;
    QString category() const override;
    int order() const override;

    void initialize() override;
    void cleanup() override;

    QObject* getController() override;

private:
    PluginLogger *m_logger = nullptr;
    ImageCropController *m_controller;
};