#pragma once

#include "../../src/core/PluginInterface.h"

class VideoSubtitleController;
class VideoSubtitleSettings;
class PluginLogger;

class VideoSubtitlePlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    static constexpr const char* kIniSection = "video-subtitle";

    VideoSubtitlePlugin(QObject *parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString iconName() const override;
    int order() const override;

    void initialize() override;
    void cleanup() override;

    QObject* getController() override;
    QObject* getSettings() override;

private:
    PluginLogger *m_logger = nullptr;
    VideoSubtitleController *m_controller;
    VideoSubtitleSettings *m_settings;
};
