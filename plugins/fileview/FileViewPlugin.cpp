#include "FileViewPlugin.h"
#include "FileViewSettings.h"
#include "Logger.h"

FileViewPlugin::FileViewPlugin(QObject *parent)
    : QObject(parent)
{
}

QString FileViewPlugin::id() const
{
    return "FileView";
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
        m_logger = new PluginLogger("FileView");
    if (!m_settings) {
        m_settings = new FileViewSettings(this);
    }
    if (!m_controller) {
        m_controller = new FileViewController(m_logger, m_settings, this);
    }
    m_logger->info(QStringLiteral("文件浏览器插件已初始化"));
}

void FileViewPlugin::cleanup()
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

QObject* FileViewPlugin::getController()
{
    if (m_controller) {
        m_controller->reset();
    }
    return m_controller;
}

QObject* FileViewPlugin::getSettings()
{
    return m_settings;
}

bool FileViewPlugin::needsMpv() const
{
    return true;
}
