#include "SubtitleAdjustPlugin.h"
#include "SubtitleAdjustSettings.h"
#include "Logger.h"

SubtitleAdjustPlugin::SubtitleAdjustPlugin(QObject *parent)
    : QObject(parent)
{
}

SubtitleAdjustPlugin::~SubtitleAdjustPlugin() = default;

QString SubtitleAdjustPlugin::id() const
{
    return PluginKey;
}

QString SubtitleAdjustPlugin::name() const
{
    return QStringLiteral("字幕时间调整");
}

QString SubtitleAdjustPlugin::description() const
{
    return QStringLiteral("实时调整字幕时间点以匹配视频");
}

QString SubtitleAdjustPlugin::iconName() const
{
    return "eye";
}

QString SubtitleAdjustPlugin::category() const
{
    return QStringLiteral("字幕处理");
}

int SubtitleAdjustPlugin::order() const
{
    return 6;
}

void SubtitleAdjustPlugin::initialize()
{
    if (!m_logger)
        m_logger = std::make_unique<PluginLogger>(PluginKey);
    if (!m_settings)
        m_settings = std::make_unique<SubtitleAdjustSettings>(this);
    if (!m_controller)
        m_controller = std::make_unique<SubtitleAdjustController>(m_logger.get(), m_settings.get(), this);
    m_logger->info(QStringLiteral("字幕时间调整插件已初始化"));
}

void SubtitleAdjustPlugin::cleanup()
{
    m_controller.reset();
    m_settings.reset();
    m_logger.reset();
}

QObject* SubtitleAdjustPlugin::getController()
{
    if (m_controller)
        m_controller->reset();
    return m_controller.get();
}

QObject* SubtitleAdjustPlugin::getSettings()
{
    return m_settings.get();
}

bool SubtitleAdjustPlugin::needsMpv() const
{
    return true;
}
