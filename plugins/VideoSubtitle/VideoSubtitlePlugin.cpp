#include "VideoSubtitlePlugin.h"
#include "VideoSubtitleController.h"
#include "VideoSubtitleSettings.h"
#include "Logger.h"

VideoSubtitlePlugin::VideoSubtitlePlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
    , m_settings(nullptr)
{
}

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
        m_logger = new PluginLogger(PluginKey);
    if (!m_settings) {
        m_settings = new VideoSubtitleSettings(m_logger, this);
    }
    if (!m_controller) {
        m_controller = new VideoSubtitleController(m_logger, m_settings, this);
    }
    m_logger->info(QStringLiteral("视频字幕翻译插件已初始化"));
}

void VideoSubtitlePlugin::cleanup()
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

QObject* VideoSubtitlePlugin::getController()
{
    if (m_controller) {
        // 如果任务正在执行中（用户选择了后台继续运行），不 reset 以免杀死后台进程
        if (!m_controller->isProcessing()) {
            m_controller->reset();
        }
    }
    return m_controller;
}

QObject* VideoSubtitlePlugin::getSettings()
{
    return m_settings;
}
