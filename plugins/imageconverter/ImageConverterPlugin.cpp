#include "ImageConverterPlugin.h"
#include "ImageConverterController.h"
#include "Logger.h"

ImageConverterPlugin::ImageConverterPlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
{
}

QString ImageConverterPlugin::id() const
{
    return "image-converter";
}

QString ImageConverterPlugin::name() const
{
    return QStringLiteral("图片格式转换");
}

QString ImageConverterPlugin::description() const
{
    return QStringLiteral("批量转换图片格式，支持递归处理");
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
        m_logger = new PluginLogger("image-converter");
    if (!m_controller) {
        m_controller = new ImageConverterController(m_logger, this);
    }
    m_logger->info(QStringLiteral("图片格式转换插件已初始化"));
}

void ImageConverterPlugin::cleanup()
{
    if (m_controller) {
        delete m_controller;
        m_controller = nullptr;
    }
    delete m_logger;
    m_logger = nullptr;
}

QObject* ImageConverterPlugin::getController()
{
    if (m_controller) {
        m_controller->reset();
    }
    return m_controller;
}
