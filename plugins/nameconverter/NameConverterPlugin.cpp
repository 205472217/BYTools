#include "NameConverterPlugin.h"

NameConverterPlugin::NameConverterPlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
{
}

QString NameConverterPlugin::id() const
{
    return "name-converter";
}

QString NameConverterPlugin::name() const
{
    return QStringLiteral("文件名繁转简");
}

QString NameConverterPlugin::description() const
{
    return QStringLiteral("批量转换文件、文件夹名称");
}

QString NameConverterPlugin::iconName() const
{
    return "translate";
}

void NameConverterPlugin::initialize()
{
    if (!m_controller) {
        m_controller = new BatchNameController(this);
    }
}

void NameConverterPlugin::cleanup()
{
    if (m_controller) {
        delete m_controller;
        m_controller = nullptr;
    }
}

QObject* NameConverterPlugin::getController()
{
    return m_controller;
}