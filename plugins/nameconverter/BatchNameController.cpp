#include "BatchNameController.h"
#include "Logger.h"

#include <QDir>
#include <QUrl>

BatchNameController::BatchNameController(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_service(m_converter, logger)
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

bool BatchNameController::isProcessing() const
{
    return m_isProcessing;
}

void BatchNameController::cancel()
{
    // 繁转简任务同步执行，执行时间短，cancel 仅用于状态同步
    if (m_isProcessing) {
        setIsProcessing(false);
        setStatusMessage("已取消");
    }
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

    m_logger->info(QString("预览繁转简: %1 (递归=%2)").arg(m_rootPath).arg(m_recursive));
    const auto items = m_service.preview(m_rootPath, currentTargetType(), m_recursive);
    m_previewModel.setItems(items);
    emit hasRecordsChanged();

    if (items.isEmpty()) {
        setStatusMessage(QStringLiteral("没有发现需要转换的名称"));
        m_logger->info("预览完成: 没有发现需要转换的名称");
    } else {
        setStatusMessage(QStringLiteral("发现 %1 项可转换名称").arg(items.count()));
        m_logger->info(QString("预览完成: 发现 %1 项可转换名称").arg(items.count()));
    }
}

void BatchNameController::executeRename()
{
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        m_previewModel.clear();
        emit hasRecordsChanged();
        setStatusMessage(QStringLiteral("请选择有效的根文件夹"));
        m_logger->warn("繁转简失败: 源文件夹无效");
        return;
    }

    m_logger->info(QString("===== 开始繁转简 ====="));
    m_logger->info(QString("根目录: %1, 递归=%2").arg(m_rootPath).arg(m_recursive));

    setIsProcessing(true);
    const auto execution = m_service.execute(m_rootPath, currentTargetType(), m_recursive);
    m_previewModel.setItems(execution.records);
    emit hasRecordsChanged();
    setStatusMessage(execution.result.message);
    m_logger->info("繁转简完成: " + execution.result.message);
    setIsProcessing(false);
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
    m_logger->info(QString("还原: %1 → %2 — %3")
        .arg(item.newName, item.currentName, result.message));
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

void BatchNameController::setIsProcessing(bool processing)
{
    if (m_isProcessing == processing) return;
    m_isProcessing = processing;
    emit isProcessingChanged();
}