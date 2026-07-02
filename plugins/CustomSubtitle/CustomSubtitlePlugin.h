#pragma once

#include "../../src/core/PluginInterface.h"

class CustomSubtitleController;
class CustomSubtitleSettings;
class PluginLogger;

class CustomSubtitlePlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    static constexpr const char* PluginKey = "CustomSubtitle";

    explicit CustomSubtitlePlugin(QObject *parent = nullptr);

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
    CustomSubtitleController *m_controller = nullptr;
    CustomSubtitleSettings *m_settings = nullptr;
};
