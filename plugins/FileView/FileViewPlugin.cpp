#include "FileViewPlugin.h"
#include "FileViewSettings.h"
#include "Logger.h"

FileViewPlugin::FileViewPlugin(QObject *parent)
    : QObject(parent)
{
}

FileViewPlugin::~FileViewPlugin() = default;

QString FileViewPlugin::id() const
{
    return PluginKey;
}

QString FileViewPlugin::name() const
{
    return QStringLiteral("文件浏览器");
}

QString FileViewPlugin::description() const
{
    return QStringLiteral("浏览指定文件夹下的任意类型文件");
}

QString FileViewPlugin::iconName() const
{
    return "eye";
}

QString FileViewPlugin::category() const
{
    return QStringLiteral("文件处理");
}

int FileViewPlugin::order() const
{
    return 7;
}

void FileViewPlugin::initialize()
{
    if (!m_logger)
        m_logger = std::make_unique<PluginLogger>(PluginKey);
    if (!m_settings)
        m_settings = std::make_unique<FileViewSettings>(this);
    if (!m_controller)
        m_controller = std::make_unique<FileViewController>(m_logger.get(), m_settings.get(), this);
    m_logger->info(QStringLiteral("文件浏览器插件已初始化"));
}

void FileViewPlugin::cleanup()
{
    m_controller.reset();
    m_settings.reset();
    m_logger.reset();
}

QObject* FileViewPlugin::getController()
{
    if (m_controller)
        m_controller->reset();
    return m_controller.get();
}

QObject* FileViewPlugin::getSettings()
{
    return m_settings.get();
}

bool FileViewPlugin::needsMpv() const
{
    return true;
}
