#include "VideoSubtitlePlugin.h"
#include "VideoSubtitleController.h"
#include "VideoSubtitleSettings.h"

VideoSubtitlePlugin::VideoSubtitlePlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
    , m_settings(nullptr)
{
}

QString VideoSubtitlePlugin::id() const
{
    return "video-subtitle";
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

void VideoSubtitlePlugin::initialize()
{
    if (!m_settings) {
        m_settings = new VideoSubtitleSettings(this);
    }
    if (!m_controller) {
        m_controller = new VideoSubtitleController(this);
    }
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
}

QObject* VideoSubtitlePlugin::getController()
{
    if (m_controller) {
        m_controller->reset();
    }
    return m_controller;
}

QObject* VideoSubtitlePlugin::getSettings()
{
    return m_settings;
}
