#include "BatchRenameController.h"

#include <QDir>
#include <QUrl>

BatchRenameController::BatchRenameController(QObject *parent)
    : QObject(parent)
    , m_service(m_converter)
{
}

QString BatchRenameController::rootPath() const
{
    return m_rootPath;
}

void BatchRenameController::setRootPath(const QString &rootPath)
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

int BatchRenameController::targetType() const
{
    return m_targetType;
}

void BatchRenameController::setTargetType(int targetType)
{
    if (m_targetType == targetType) {
        return;
    }

    m_targetType = targetType;
    emit targetTypeChanged();
}

QString BatchRenameController::statusMessage() const
{
    return m_statusMessage;
}

bool BatchRenameController::hasRecords() const
{
    return m_previewModel.rowCount() > 0;
}

RenamePreviewModel *BatchRenameController::previewModel()
{
    return &m_previewModel;
}

void BatchRenameController::buildPreview()
{
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        m_previewModel.clear();
        emit hasRecordsChanged();
        setStatusMessage(QStringLiteral("请选择有效的根文件夹"));
        return;
    }

    const auto items = m_service.preview(m_rootPath, currentTargetType());
    m_previewModel.setItems(items);
    emit hasRecordsChanged();

    if (items.isEmpty()) {
        setStatusMessage(QStringLiteral("没有发现需要转换的名称"));
    } else {
        setStatusMessage(QStringLiteral("发现 %1 项可转换名称").arg(items.count()));
    }
}

void BatchRenameController::executeRename()
{
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        m_previewModel.clear();
        emit hasRecordsChanged();
        setStatusMessage(QStringLiteral("请选择有效的根文件夹"));
        return;
    }

    const auto execution = m_service.execute(m_rootPath, currentTargetType());
    m_previewModel.setItems(execution.records);
    emit hasRecordsChanged();
    setStatusMessage(execution.result.message);
}

void BatchRenameController::restoreRecord(int row)
{
    const auto item = m_previewModel.itemAt(row);
    const auto result = m_service.restore(item);
    m_previewModel.updateStatus(row, result.success ? QStringLiteral("已还原") : QStringLiteral("还原失败"));
    if (result.success && item.directory) {
        m_previewModel.replacePathPrefix(item.newPath, item.currentPath);
    }
    setStatusMessage(result.message);
}

void BatchRenameController::clearRecords()
{
    m_previewModel.clear();
    emit hasRecordsChanged();
    setStatusMessage({});
}

RenameService::TargetType BatchRenameController::currentTargetType() const
{
    switch (m_targetType) {
    case 0:
        return RenameService::TargetType::Files;
    case 1:
        return RenameService::TargetType::Directories;
    default:
        return RenameService::TargetType::FilesAndDirectories;
    }
}

void BatchRenameController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}
