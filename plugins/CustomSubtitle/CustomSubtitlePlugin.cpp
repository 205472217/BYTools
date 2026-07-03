#include "CustomSubtitlePlugin.h"
#include "CustomSubtitleController.h"
#include "CustomSubtitleSettings.h"
#include "Logger.h"

CustomSubtitlePlugin::CustomSubtitlePlugin(QObject *parent)
    : QObject(parent)
{
}

CustomSubtitlePlugin::~CustomSubtitlePlugin() = default;

QString CustomSubtitlePlugin::id() const
{
    return PluginKey;
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
    return PluginKey;
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
        m_logger = std::make_unique<PluginLogger>(PluginKey);
    if (!m_settings)
        m_settings = std::make_unique<CustomSubtitleSettings>(this);
    if (!m_controller)
        m_controller = std::make_unique<CustomSubtitleController>(m_logger.get(), m_settings.get(), this);
    m_logger->info(QStringLiteral("自定义视频字幕插件已初始化"));
}

void CustomSubtitlePlugin::cleanup()
{
    m_controller.reset();
    m_settings.reset();
    m_logger.reset();
}

QObject* CustomSubtitlePlugin::getController()
{
    if (m_controller) {
        if (!m_controller->isProcessing())
            m_controller->reset();
    }
    return m_controller.get();
}

QObject* CustomSubtitlePlugin::getSettings()
{
    return m_settings.get();
}
