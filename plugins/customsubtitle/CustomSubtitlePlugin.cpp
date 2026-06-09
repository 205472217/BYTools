#include "CustomSubtitlePlugin.h"
#include "CustomSubtitleController.h"

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
    return QStringLiteral("从网站下载字幕，匹配视频并合成，替换原视频");
}

QString CustomSubtitlePlugin::iconName() const
{
    return "custom-subtitle";
}

void CustomSubtitlePlugin::initialize()
{
    if (!m_controller) {
        m_controller = new CustomSubtitleController(this);
    }
}

void CustomSubtitlePlugin::cleanup()
{
    if (m_controller) {
        delete m_controller;
        m_controller = nullptr;
    }
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
