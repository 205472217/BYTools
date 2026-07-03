#include "BatchRenamePlugin.h"
#include "BatchRenameController.h"
#include "BatchRenameSettings.h"
#include "Logger.h"

BatchRenamePlugin::BatchRenamePlugin(QObject *parent)
    : QObject(parent)
{
}

BatchRenamePlugin::~BatchRenamePlugin() = default;

QString BatchRenamePlugin::id() const
{
    return PluginKey;
}

QString BatchRenamePlugin::name() const
{
    return QStringLiteral("批量重命名");
}

QString BatchRenamePlugin::description() const
{
    return QStringLiteral("批量重命名文件，支持多种命名规则");
}

QString BatchRenamePlugin::iconName() const
{
    return "rename";
}

QString BatchRenamePlugin::category() const
{
    return QStringLiteral("文件处理");
}

int BatchRenamePlugin::order() const
{
    return 1;
}

void BatchRenamePlugin::initialize()
{
    if (!m_logger)
        m_logger = std::make_unique<PluginLogger>(PluginKey);
    if (!m_settings)
        m_settings = std::make_unique<BatchRenameSettings>(this);
    if (!m_controller)
        m_controller = std::make_unique<BatchRenameController>(m_logger.get(), m_settings.get(), this);
    m_logger->info(QStringLiteral("批量重命名插件已初始化"));
}

void BatchRenamePlugin::cleanup()
{
    m_controller.reset();
    m_settings.reset();
    m_logger.reset();
}

QObject* BatchRenamePlugin::getController()
{
    if (m_controller && !m_controller->isProcessing())
        m_controller->reset();
    return m_controller.get();
}

QObject* BatchRenamePlugin::getSettings()
{
    return m_settings.get();
}
