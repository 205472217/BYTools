#pragma once

#include <memory>
#include "../../src/core/PluginInterface.h"

class ImageConverterController;
class ImageConverterSettings;
class PluginLogger;

class ImageConverterPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    static constexpr const char* PluginKey = "ImageConverter";

    ImageConverterPlugin(QObject *parent = nullptr);
    ~ImageConverterPlugin() override;

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
    std::unique_ptr<ImageConverterController> m_controller;
    std::unique_ptr<ImageConverterSettings> m_settings;
};
