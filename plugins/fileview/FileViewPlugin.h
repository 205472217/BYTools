#pragma once

#include "../../src/core/PluginInterface.h"
#include "FileViewController.h"

class PluginLogger;
class FileViewSettings;

class FileViewPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    FileViewPlugin(QObject *parent = nullptr);

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
    FileViewController *m_controller = nullptr;
    FileViewSettings *m_settings = nullptr;
};
