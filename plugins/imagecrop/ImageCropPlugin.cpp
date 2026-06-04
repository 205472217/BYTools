#include "ImageCropPlugin.h"
#include "ImageCropController.h"

ImageCropPlugin::ImageCropPlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
{
}

QString ImageCropPlugin::id() const
{
    return "image-crop";
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

void ImageCropPlugin::initialize()
{
    if (!m_controller) {
        m_controller = new ImageCropController(this);
    }
}

void ImageCropPlugin::cleanup()
{
    if (m_controller) {
        delete m_controller;
        m_controller = nullptr;
    }
}

QObject* ImageCropPlugin::getController()
{
    if (m_controller) {
        m_controller->reset();
    }
    return m_controller;
}