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
    , m_service(logger)
{
    m_previewModel = new NamePreviewModel(this);

    connect(&m_workerThread, &QThread::finished, this, [this]() {
        m_workerRunning = false;
    });
}

NameConverterController::~NameConverterController()
{
    cancel();
    m_workerThread.quit();
    m_workerThread.wait(5000);
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

QString NameConverterController::rootPath() const { return m_settings->rootPath(); }
void NameConverterController::setRootPath(const QString &path) {
    if (m_settings->rootPath() != path) {
        m_settings->setRootPath(path);
        emit rootPathChanged();
    }
}
int NameConverterController::targetType() const { return m_settings->targetType(); }
void NameConverterController::setTargetType(int type) {
    if (m_settings->targetType() != type) {
        m_settings->setTargetType(type);
        emit targetTypeChanged();
    }
}
bool NameConverterController::recursive() const { return m_settings->recursive(); }
void NameConverterController::setRecursive(bool rec) {
    if (m_settings->recursive() != rec) {
        m_settings->setRecursive(rec);
        emit recursiveChanged();
    }
}

void NameConverterController::cancel()
{
    if (m_workerRunning) {
        m_workerThread.quit();
        if (!m_workerThread.wait(3000)) {
            m_workerThread.terminate();
            m_workerThread.wait(3000);
        }
    }
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
    emit logMessage(QString("正在扫描: %1").arg(root));
    if (rec)
        emit logMessage("  启用了递归查找，正在遍历子目录...");

    setIsProcessing(true);

    // Disconnect any previous started connection, then connect for preview
    disconnect(&m_workerThread, &QThread::started, nullptr, nullptr);
    connect(&m_workerThread, &QThread::started, this, [this]() {
        const QString root = m_settings->rootPath();
        const bool rec = m_settings->recursive();
        auto items = m_service.preview(root, currentTargetType(), rec);
        m_workerThread.quit();
        QMetaObject::invokeMethod(this, [this, items]() {
            m_previewModel->setItems(items);
            emit hasRecordsChanged();
            if (items.isEmpty()) {
                setStatusMessage(QStringLiteral("没有发现需要转换的名称"));
                m_logger->info("预览完成: 没有发现需要转换的名称");
            } else {
                setStatusMessage(QStringLiteral("发现 %1 项可转换名称").arg(items.count()));
                m_logger->info(QString("预览完成: 发现 %1 项可转换名称").arg(items.count()));
            }
            setIsProcessing(false);
        }, Qt::QueuedConnection);
    }, Qt::DirectConnection);

    m_workerRunning = true;
    m_workerThread.start();
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
    emit logMessage(QString("根目录: %1").arg(root));
    if (rec)
        emit logMessage("  启用了递归查找，正在遍历子目录...");

    // Ensure model is populated from preview before execution starts
    if (m_previewModel->rowCount() == 0) {
        auto items = m_service.preview(root, currentTargetType(), rec);
        m_previewModel->setItems(items);
        emit hasRecordsChanged();
    }

    setIsProcessing(true);

    disconnect(&m_workerThread, &QThread::started, nullptr, nullptr);
    connect(&m_workerThread, &QThread::started, this, [this]() {
        const QString root = m_settings->rootPath();
        const bool rec = m_settings->recursive();
        auto execution = m_service.execute(root, currentTargetType(), rec,
            [this](int index, const NamePreviewItem &item) {
                QMetaObject::invokeMethod(m_previewModel, [this, index, item]() {
                    if (index < m_previewModel->rowCount()) {
                        m_previewModel->updateStatus(index, item.status);
                        if (item.directory && item.status == QStringLiteral("已转换")) {
                            m_previewModel->replacePathPrefix(item.currentPath, item.newPath);
                        }
                    }
                }, Qt::QueuedConnection);
            });
        m_workerThread.quit();
        QMetaObject::invokeMethod(this, [this, execution]() {
            setStatusMessage(execution.result.message);
            m_logger->info("繁转简完成: " + execution.result.message);
            setIsProcessing(false);
        }, Qt::QueuedConnection);
    }, Qt::DirectConnection);

    m_workerRunning = true;
    m_workerThread.start();
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
    cancel();
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
