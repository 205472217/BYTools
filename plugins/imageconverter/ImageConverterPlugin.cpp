#include "ImageConverterPlugin.h"
#include "ImageConverterController.h"

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

void ImageConverterPlugin::initialize()
{
    if (!m_controller) {
        m_controller = new ImageConverterController(this);
    }
}

void ImageConverterPlugin::cleanup()
{
    if (m_controller) {
        delete m_controller;
        m_controller = nullptr;
    }
}

QObject* ImageConverterPlugin::getController()
{
    if (m_controller) {
        m_controller->reset();
    }
    return m_controller;
}
