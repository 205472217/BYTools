#include "NameConverterController.h"
#include "NameConverterSettings.h"
#include "Config.h"
#include "Logger.h"
#include "SettingsHelper.h"

#include <QDir>
#include <QUrl>

NameConverterController::NameConverterController(PluginLogger *logger, NameConverterSettings *settings, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(settings)
    , m_service(m_converter, logger)
{
    m_previewModel = new NamePreviewModel(this);
}

QString NameConverterController::statusMessage() const
{
    return m_statusMessage;
}

bool NameConverterController::hasRecords() const
{
    return m_previewModel->rowCount() > 0;
}

bool NameConverterController::isProcessing() const
{
    return m_isProcessing;
}

void NameConverterController::cancel()
{
    // 繁转简任务同步执行，执行时间短，cancel 仅用于状态同步
    if (m_isProcessing) {
        setIsProcessing(false);
        setStatusMessage("已取消");
    }
}

NamePreviewModel* NameConverterController::previewModel() const
{
    return m_previewModel;
}

void NameConverterController::buildPreview()
{
    const QString root = m_settings->rootPath();
    const bool rec = m_settings->recursive();
    if (root.isEmpty() || !QDir(root).exists()) {
        m_previewModel->clear();
        emit hasRecordsChanged();
        setStatusMessage(QStringLiteral("请选择有效的源文件夹"));
        return;
    }

    m_logger->info(QString("预览繁转简: %1 (递归=%2)").arg(root).arg(rec));
    const auto items = m_service.preview(root, currentTargetType(), rec);
    m_previewModel->setItems(items);
    emit hasRecordsChanged();

    if (items.isEmpty()) {
        setStatusMessage(QStringLiteral("没有发现需要转换的名称"));
        m_logger->info("预览完成: 没有发现需要转换的名称");
    } else {
        setStatusMessage(QStringLiteral("发现 %1 项可转换名称").arg(items.count()));
        m_logger->info(QString("预览完成: 发现 %1 项可转换名称").arg(items.count()));
    }
}

void NameConverterController::executeRename()
{
    const QString root = m_settings->rootPath();
    const bool rec = m_settings->recursive();
    if (root.isEmpty() || !QDir(root).exists()) {
        m_previewModel->clear();
        emit hasRecordsChanged();
        setStatusMessage(QStringLiteral("请选择有效的源文件夹"));
        m_logger->warn("繁转简失败: 源文件夹无效");
        return;
    }

    m_logger->info(QString("===== 开始繁转简 ====="));
    m_logger->info(QString("根目录: %1, 递归=%2").arg(root).arg(rec));

    setIsProcessing(true);
    const auto execution = m_service.execute(root, currentTargetType(), rec);
    m_previewModel->setItems(execution.records);
    emit hasRecordsChanged();
    setStatusMessage(execution.result.message);
    m_logger->info("繁转简完成: " + execution.result.message);
    setIsProcessing(false);
}

void NameConverterController::restoreRecord(int row)
{
    const auto item = m_previewModel->itemAt(row);
    const auto result = m_service.restore(item);
    m_previewModel->updateStatus(row, result.success ? QStringLiteral("已还原") : QStringLiteral("还原失败"));
    if (result.success && item.directory) {
        m_previewModel->replacePathPrefix(item.newPath, item.currentPath);
    }
    setStatusMessage(result.message);
    m_logger->info(QString("还原: %1 → %2 — %3")
        .arg(item.newName, item.currentName, result.message));
}

void NameConverterController::restoreAllRecords()
{
    m_logger->info("===== 开始批量还原 =====");
    int successCount = 0;
    int failCount = 0;

    const int total = m_previewModel->rowCount();
    for (int i = 0; i < total; ++i) {
        const auto item = m_previewModel->itemAt(i);
        if (item.status != QStringLiteral("已转换"))
            continue;

        const auto result = m_service.restore(item);
        m_previewModel->updateStatus(i, result.success ? QStringLiteral("已还原") : QStringLiteral("还原失败"));
        if (result.success && item.directory) {
            m_previewModel->replacePathPrefix(item.newPath, item.currentPath);
        }
        if (result.success)
            successCount++;
        else
            failCount++;
    }

    if (successCount == 0 && failCount == 0) {
        setStatusMessage(QStringLiteral("没有可还原的记录"));
        m_logger->info("批量还原完成: 没有可还原的记录");
    } else if (failCount == 0) {
        setStatusMessage(QStringLiteral("已全部还原"));
        m_logger->info(QString("批量还原完成: 成功还原 %1 个").arg(successCount));
    } else {
        setStatusMessage(QStringLiteral("成功还原 %1 个，失败 %2 个").arg(successCount).arg(failCount));
        m_logger->info(QString("批量还原完成: 成功 %1 个，失败 %2 个").arg(successCount).arg(failCount));
    }
}

void NameConverterController::clearRecords()
{
    m_previewModel->clear();
    emit hasRecordsChanged();
    setStatusMessage({});
}

void NameConverterController::reset()
{
    clearRecords();
}

NameService::TargetType NameConverterController::currentTargetType() const
{
    switch (m_settings->targetType()) {
    case 0:
        return NameService::TargetType::Files;
    case 1:
        return NameService::TargetType::Directories;
    default:
        return NameService::TargetType::FilesAndDirectories;
    }
}

void NameConverterController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}

void NameConverterController::setIsProcessing(bool processing)
{
    if (m_isProcessing == processing) return;
    m_isProcessing = processing;
    emit isProcessingChanged();
}
