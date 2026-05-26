#include "BatchNameController.h"

#include <QDir>
#include <QUrl>

BatchNameController::BatchNameController(QObject *parent)
    : QObject(parent)
    , m_service(m_converter)
{
}

QString BatchNameController::rootPath() const
{
    return m_rootPath;
}

void BatchNameController::setRootPath(const QString &rootPath)
{
    QString normalizedPath = rootPath;
    const QUrl url(rootPath);
    if (url.isLocalFile()) {
        normalizedPath = url.toLocalFile();
    }

    if (m_rootPath == normalizedPath) {
        return;
    }

    m_rootPath = normalizedPath;
    emit rootPathChanged();
}

int BatchNameController::targetType() const
{
    return m_targetType;
}

void BatchNameController::setTargetType(int targetType)
{
    if (m_targetType == targetType) {
        return;
    }

    m_targetType = targetType;
    emit targetTypeChanged();
}

bool BatchNameController::recursive() const
{
    return m_recursive;
}

void BatchNameController::setRecursive(bool recursive)
{
    if (m_recursive == recursive) return;
    m_recursive = recursive;
    emit recursiveChanged();
}

QString BatchNameController::statusMessage() const
{
    return m_statusMessage;
}

bool BatchNameController::hasRecords() const
{
    return m_previewModel.rowCount() > 0;
}

QObject* BatchNameController::previewModel()
{
    return &m_previewModel;
}

void BatchNameController::buildPreview()
{
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        m_previewModel.clear();
        emit hasRecordsChanged();
        setStatusMessage(QStringLiteral("请选择有效的根文件夹"));
        return;
    }

    const auto items = m_service.preview(m_rootPath, currentTargetType(), m_recursive);
    m_previewModel.setItems(items);
    emit hasRecordsChanged();

    if (items.isEmpty()) {
        setStatusMessage(QStringLiteral("没有发现需要转换的名称"));
    } else {
        setStatusMessage(QStringLiteral("发现 %1 项可转换名称").arg(items.count()));
    }
}

void BatchNameController::executeRename()
{
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        m_previewModel.clear();
        emit hasRecordsChanged();
        setStatusMessage(QStringLiteral("请选择有效的根文件夹"));
        return;
    }

    const auto execution = m_service.execute(m_rootPath, currentTargetType(), m_recursive);
    m_previewModel.setItems(execution.records);
    emit hasRecordsChanged();
    setStatusMessage(execution.result.message);
}

void BatchNameController::restoreRecord(int row)
{
    const auto item = m_previewModel.itemAt(row);
    const auto result = m_service.restore(item);
    m_previewModel.updateStatus(row, result.success ? QStringLiteral("已还原") : QStringLiteral("还原失败"));
    if (result.success && item.directory) {
        m_previewModel.replacePathPrefix(item.newPath, item.currentPath);
    }
    setStatusMessage(result.message);
}

void BatchNameController::clearRecords()
{
    m_previewModel.clear();
    emit hasRecordsChanged();
    setStatusMessage({});
}

void BatchNameController::reset()
{
    clearRecords();
    m_rootPath.clear();
    m_targetType = 2;
    m_recursive = false;
    setStatusMessage({});
    emit rootPathChanged();
    emit targetTypeChanged();
    emit recursiveChanged();
}

NameService::TargetType BatchNameController::currentTargetType() const
{
    switch (m_targetType) {
    case 0:
        return NameService::TargetType::Files;
    case 1:
        return NameService::TargetType::Directories;
    default:
        return NameService::TargetType::FilesAndDirectories;
    }
}

void BatchNameController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}