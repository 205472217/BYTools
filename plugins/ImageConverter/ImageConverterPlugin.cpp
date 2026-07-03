#include "ImageConverterPlugin.h"
#include "ImageConverterController.h"
#include "ImageConverterSettings.h"
#include "Logger.h"

ImageConverterPlugin::ImageConverterPlugin(QObject *parent)
    : QObject(parent)
{
}

ImageConverterPlugin::~ImageConverterPlugin() = default;

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
        m_logger = std::make_unique<PluginLogger>(PluginKey);
    if (!m_settings)
        m_settings = std::make_unique<ImageConverterSettings>(this);
    if (!m_controller)
        m_controller = std::make_unique<ImageConverterController>(m_logger.get(), m_settings.get(), this);
    m_logger->info(QStringLiteral("图片处理插件已初始化"));
}

void ImageConverterPlugin::cleanup()
{
    m_controller.reset();
    m_settings.reset();
    m_logger.reset();
}

QObject* ImageConverterPlugin::getController()
{
    if (m_controller && !m_controller->isProcessing())
        m_controller->reset();
    return m_controller.get();
}

QObject* ImageConverterPlugin::getSettings()
{
    return m_settings.get();
}
