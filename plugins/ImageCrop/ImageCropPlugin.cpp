#include "ImageCropPlugin.h"
#include "ImageCropController.h"
#include "ImageCropSettings.h"
#include "Logger.h"

ImageCropPlugin::ImageCropPlugin(QObject *parent)
    : QObject(parent)
{
}

ImageCropPlugin::~ImageCropPlugin() = default;

QString ImageCropPlugin::id() const
{
    return PluginKey;
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
        m_logger = std::make_unique<PluginLogger>(PluginKey);
    if (!m_settings)
        m_settings = std::make_unique<ImageCropSettings>(this);
    if (!m_controller)
        m_controller = std::make_unique<ImageCropController>(m_logger.get(), m_settings.get(), this);
    m_logger->info(QStringLiteral("图片裁剪插件已初始化"));
}

void ImageCropPlugin::cleanup()
{
    m_controller.reset();
    m_settings.reset();
    m_logger.reset();
}

QObject* ImageCropPlugin::getController()
{
    if (m_controller && !m_controller->isProcessing())
        m_controller->reset();
    return m_controller.get();
}

QObject* ImageCropPlugin::getSettings()
{
    return m_settings.get();
}
