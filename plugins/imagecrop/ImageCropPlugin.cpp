#include "ImageCropPlugin.h"
#include "ImageCropController.h"
#include "ImageCropSettings.h"
#include "Logger.h"

ImageCropPlugin::ImageCropPlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
{
}

QString ImageCropPlugin::id() const
{
    return "ImageCrop";
}

QString ImageCropPlugin::name() const
{
    return QStringLiteral("图片裁剪");
}

QString ImageCropPlugin::description() const
{
    return QStringLiteral("按比例或像素裁剪图片，支持批量处理");
}

QString ImageCropPlugin::iconName() const
{
    return "crop";
}

QString ImageCropPlugin::category() const
{
    return QStringLiteral("图片处理");
}

int ImageCropPlugin::order() const
{
    return 3;
}

void ImageCropPlugin::initialize()
{
    if (!m_logger)
        m_logger = new PluginLogger("ImageCrop");
    if (!m_settings) {
        m_settings = new ImageCropSettings(this);
    }
    if (!m_controller) {
        m_controller = new ImageCropController(m_logger, m_settings, this);
    }
    m_logger->info(QStringLiteral("图片裁剪插件已初始化"));
}

void ImageCropPlugin::cleanup()
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

QObject* ImageCropPlugin::getController()
{
    if (m_controller && !m_controller->isProcessing()) {
        m_controller->reset();
    }
    return m_controller;
}

QObject* ImageCropPlugin::getSettings()
{
    return m_settings;
}