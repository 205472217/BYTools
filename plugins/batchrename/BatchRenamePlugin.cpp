#include "BatchRenamePlugin.h"
#include "BatchRenameController.h"
#include "BatchRenameSettings.h"
#include "Logger.h"

BatchRenamePlugin::BatchRenamePlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
{
}

QString BatchRenamePlugin::id() const
{
    return "batch-rename";
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
        m_logger = new PluginLogger("batch-rename");
    if (!m_settings) {
        m_settings = new BatchRenameSettings(this);
    }
    if (!m_controller) {
        m_controller = new BatchRenameController(m_logger, m_settings, this);
    }
    m_logger->info(QStringLiteral("批量重命名插件已初始化"));
}

void BatchRenamePlugin::cleanup()
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

QObject* BatchRenamePlugin::getController()
{
    if (m_controller && !m_controller->isProcessing()) {
        m_controller->reset();
    }
    return m_controller;
}

QObject* BatchRenamePlugin::getSettings()
{
    return m_settings;
}