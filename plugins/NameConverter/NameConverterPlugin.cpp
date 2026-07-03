#include "NameConverterPlugin.h"
#include "NameConverterSettings.h"
#include "Logger.h"

NameConverterPlugin::NameConverterPlugin(QObject *parent)
    : QObject(parent)
{
}

NameConverterPlugin::~NameConverterPlugin() = default;

QString NameConverterPlugin::id() const
{
    return PluginKey;
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
        m_logger = std::make_unique<PluginLogger>(PluginKey);
    if (!m_settings)
        m_settings = std::make_unique<NameConverterSettings>(this);
    if (!m_controller)
        m_controller = std::make_unique<NameConverterController>(m_logger.get(), m_settings.get(), this);
    m_logger->info(QStringLiteral("文件名繁转简插件已初始化"));
}

void NameConverterPlugin::cleanup()
{
    m_controller.reset();
    m_settings.reset();
    m_logger.reset();
}

QObject* NameConverterPlugin::getController()
{
    if (m_controller && !m_controller->isProcessing())
        m_controller->reset();
    return m_controller.get();
}

QObject* NameConverterPlugin::getSettings()
{
    return m_settings.get();
}
