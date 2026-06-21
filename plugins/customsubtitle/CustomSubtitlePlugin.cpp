#include "CustomSubtitlePlugin.h"
#include "CustomSubtitleController.h"
#include "CustomSubtitleSettings.h"
#include "Logger.h"

CustomSubtitlePlugin::CustomSubtitlePlugin(QObject *parent)
    : QObject(parent)
{
}

QString CustomSubtitlePlugin::id() const
{
    return "custom-subtitle";
}

QString CustomSubtitlePlugin::name() const
{
    return QStringLiteral("自定义视频字幕");
}

QString CustomSubtitlePlugin::description() const
{
    return QStringLiteral("从网站下载字幕->根据关键码匹配视频字幕->将匹配的视频和字幕合成->替换原视频");
}

QString CustomSubtitlePlugin::iconName() const
{
    return "custom-subtitle";
}

QString CustomSubtitlePlugin::category() const
{
    return QStringLiteral("字幕处理");
}

int CustomSubtitlePlugin::order() const
{
    return 5;
}

void CustomSubtitlePlugin::initialize()
{
    if (!m_logger)
        m_logger = new PluginLogger("custom-subtitle");
    if (!m_settings) {
        m_settings = new CustomSubtitleSettings(this);
    }
    if (!m_controller) {
        m_controller = new CustomSubtitleController(m_logger, m_settings, this);
    }
    m_logger->info(QStringLiteral("自定义视频字幕插件已初始化"));
}

void CustomSubtitlePlugin::cleanup()
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

QObject* CustomSubtitlePlugin::getController()
{
    if (m_controller) {
        if (!m_controller->isProcessing()) {
            m_controller->reset();
        }
    }
    return m_controller;
}

QObject* CustomSubtitlePlugin::getSettings()
{
    return m_settings;
}
