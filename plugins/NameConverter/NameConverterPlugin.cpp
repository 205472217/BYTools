#include "NameConverterPlugin.h"
#include "NameConverterSettings.h"
#include "Logger.h"

NameConverterPlugin::NameConverterPlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
{
}

QString NameConverterPlugin::id() const
{
    return "NameConverter";
}

QString NameConverterPlugin::name() const
{
    return QStringLiteral("文件名繁转简");
}

QString NameConverterPlugin::description() const
{
    return QStringLiteral("批量转换文件、文件夹名称");
}

QString NameConverterPlugin::iconName() const
{
    return "translate";
}

QString NameConverterPlugin::category() const
{
    return QStringLiteral("文件处理");
}

int NameConverterPlugin::order() const
{
    return 0;
}

void NameConverterPlugin::initialize()
{
    if (!m_logger)
        m_logger = new PluginLogger("NameConverter");
    if (!m_settings) {
        m_settings = new NameConverterSettings(this);
    }
    if (!m_controller) {
        m_controller = new NameConverterController(m_logger, m_settings, this);
    }
    m_logger->info(QStringLiteral("文件名繁转简插件已初始化"));
}

void NameConverterPlugin::cleanup()
{
    if (m_controller) {
        delete m_controller;
        m_controller = nullptr;
    }
    if (m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }
    delete m_logger;
    m_logger = nullptr;
}

QObject* NameConverterPlugin::getController()
{
    if (m_controller && !m_controller->isProcessing()) {
        m_controller->reset();
    }
    return m_controller;
}

QObject* NameConverterPlugin::getSettings()
{
    return m_settings;
}