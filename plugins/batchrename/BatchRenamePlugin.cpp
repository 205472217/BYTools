#include "BatchRenamePlugin.h"
#include "BatchRenameController.h"

BatchRenamePlugin::BatchRenamePlugin(QObject *parent)
    : QObject(parent)
    , m_controller(nullptr)
{
}

QString BatchRenamePlugin::id() const
{
    return "batch-rename";
}

QString BatchRenamePlugin::name() const
{
    return QStringLiteral("批量重命名");
}

QString BatchRenamePlugin::description() const
{
    return QStringLiteral("批量重命名文件，支持多种命名规则");
}

QString BatchRenamePlugin::iconName() const
{
    return "rename";
}

void BatchRenamePlugin::initialize()
{
    if (!m_controller) {
        m_controller = new BatchRenameController(this);
    }
}

void BatchRenamePlugin::cleanup()
{
    if (m_controller) {
        delete m_controller;
        m_controller = nullptr;
    }
}

QObject* BatchRenamePlugin::getController()
{
    if (m_controller) {
        m_controller->reset();
    }
    return m_controller;
}