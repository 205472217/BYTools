#include "ImageConverterPlugin.h"
#include "ImageConverterController.h"
#include "ImageConverterSettings.h"
#include "Logger.h"

ImageConverterPlugin::ImageConverterPlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
{
}

QString ImageConverterPlugin::id() const
{
    return PluginKey;
}

QString ImageConverterPlugin::name() const
{
    return QStringLiteral("图片处理");
}

QString ImageConverterPlugin::description() const
{
    return QStringLiteral("批量转换图片格式、缩放尺寸，支持递归子文件夹");
}

QString ImageConverterPlugin::iconName() const
{
    return "image";
}

QString ImageConverterPlugin::category() const
{
    return QStringLiteral("图片处理");
}

int ImageConverterPlugin::order() const
{
    return 2;
}

void ImageConverterPlugin::initialize()
{
    if (!m_logger)
        m_logger = new PluginLogger(PluginKey);
    if (!m_settings) {
        m_settings = new ImageConverterSettings(this);
    }
    if (!m_controller) {
        m_controller = new ImageConverterController(m_logger, m_settings, this);
    }
    m_logger->info(QStringLiteral("图片处理插件已初始化"));
}

void ImageConverterPlugin::cleanup()
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

QObject* ImageConverterPlugin::getController()
{
    if (m_controller && !m_controller->isProcessing()) {
        m_controller->reset();
    }
    return m_controller;
}

QObject* ImageConverterPlugin::getSettings()
{
    return m_settings;
}
