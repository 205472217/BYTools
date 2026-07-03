#pragma once

#include <memory>
#include "../../src/core/PluginInterface.h"

class ImageCropController;
class ImageCropSettings;
class PluginLogger;

class ImageCropPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)

public:
    static constexpr const char* PluginKey = "ImageCrop";

    ImageCropPlugin(QObject *parent = nullptr);
    ~ImageCropPlugin() override;

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
    std::unique_ptr<PluginLogger> m_logger;
    std::unique_ptr<ImageCropController> m_controller;
    std::unique_ptr<ImageCropSettings> m_settings;
};
