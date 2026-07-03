#include "VideoSubtitlePlugin.h"
#include "VideoSubtitleController.h"
#include "VideoSubtitleSettings.h"
#include "Logger.h"

VideoSubtitlePlugin::VideoSubtitlePlugin(QObject *parent)
    : QObject(parent)
{
}

VideoSubtitlePlugin::~VideoSubtitlePlugin() = default;

QString VideoSubtitlePlugin::id() const
{
    return PluginKey;
}

QString VideoSubtitlePlugin::name() const
{
    return QStringLiteral("视频字幕翻译");
}

QString VideoSubtitlePlugin::description() const
{
    return QStringLiteral("提取视频音频，语音识别后翻译并烧录字幕");
}

QString VideoSubtitlePlugin::iconName() const
{
    return "subtitle";
}

QString VideoSubtitlePlugin::category() const
{
    return QStringLiteral("字幕处理");
}

int VideoSubtitlePlugin::order() const
{
    return 4;
}

void VideoSubtitlePlugin::initialize()
{
    if (!m_logger)
        m_logger = std::make_unique<PluginLogger>(PluginKey);
    if (!m_settings)
        m_settings = std::make_unique<VideoSubtitleSettings>(m_logger.get(), this);
    if (!m_controller)
        m_controller = std::make_unique<VideoSubtitleController>(m_logger.get(), m_settings.get(), this);
    m_logger->info(QStringLiteral("视频字幕翻译插件已初始化"));
}

void VideoSubtitlePlugin::cleanup()
{
    m_controller.reset();
    m_settings.reset();
    m_logger.reset();
}

QObject* VideoSubtitlePlugin::getController()
{
    if (m_controller) {
        if (!m_controller->isProcessing())
            m_controller->reset();
    }
    return m_controller.get();
}

QObject* VideoSubtitlePlugin::getSettings()
{
    return m_settings.get();
}
