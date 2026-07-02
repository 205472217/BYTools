#include "SubtitleAdjustPlugin.h"
#include "SubtitleAdjustSettings.h"
#include "Logger.h"

SubtitleAdjustPlugin::SubtitleAdjustPlugin(QObject *parent)
    : QObject(parent)
{
}

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
        m_logger = new PluginLogger(PluginKey);
    if (!m_settings) {
        m_settings = new SubtitleAdjustSettings(this);
    }
    if (!m_controller) {
        m_controller = new SubtitleAdjustController(m_logger, m_settings, this);
    }
    m_logger->info(QStringLiteral("字幕时间调整插件已初始化"));
}

void SubtitleAdjustPlugin::cleanup()
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

QObject* SubtitleAdjustPlugin::getController()
{
    if (m_controller) {
        m_controller->reset();
    }
    return m_controller;
}

QObject* SubtitleAdjustPlugin::getSettings()
{
    return m_settings;
}

bool SubtitleAdjustPlugin::needsMpv() const
{
    return true;
}
