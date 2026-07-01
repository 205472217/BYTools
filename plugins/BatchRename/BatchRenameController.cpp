#include "BatchRenameController.h"
#include "BatchRenamePlugin.h"
#include "BatchRenameSettings.h"
#include "Config.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QRegularExpression>

BatchRenameController::BatchRenameController(PluginLogger *logger, BatchRenameSettings *settings, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(settings)
{
    connect(&m_workerThread, &QThread::started,
            this, &BatchRenameController::doWork, Qt::DirectConnection);
    connect(&m_workerThread, &QThread::finished, this, [this]() {
        m_workerRunning = false;
    });
}

BatchRenameController::~BatchRenameController()
{
    cancel();
    m_workerThread.quit();
    m_workerThread.wait(5000);
}

QString BatchRenameController::statusMessage() const
{
    return m_statusMessage;
}

bool BatchRenameController::hasRecords() const
{
    return !m_records.isEmpty();
}

QVariantList BatchRenameController::records() const
{
    QVariantList result;
    for (const auto &record : m_records) {
        QVariantMap map;
        map["originalPath"] = record.originalPath;
        map["newPath"] = record.newPath;
        map["originalName"] = record.originalName;
        map["newName"] = record.newName;
        map["fileType"] = record.fileType;
        map["success"] = record.success;
        map["status"] = record.status;
        result.append(map);
    }
    return result;
}

void BatchRenameController::executeRename()
{
    QString rootPath = m_settings->rootPath();
    bool recursive = m_settings->recursive();
    int fileType = m_settings->fileType();
    QString customExtension = m_settings->customExtension();
    int renameMode = m_settings->renameMode();
    QString baseName = m_settings->baseName();
    QString searchText = m_settings->searchText();
    QString replaceText = m_settings->replaceText();

    if (rootPath.isEmpty() || !QDir(rootPath).exists()) {
        setStatusMessage(QStringLiteral("请选择有效的源文件夹"));
        return;
    }

    if (renameMode == 0 && baseName.isEmpty()) {
        setStatusMessage(QStringLiteral("请填写指定名称"));
        return;
    }

    if (renameMode == 1 && searchText.isEmpty()) {
        setStatusMessage(QStringLiteral("请填写查找文本"));
        return;
    }

    m_logger->info(QString("===== 开始批量重命名 ====="));
    m_logger->info(QString("根目录: %1, 递归=%2").arg(rootPath).arg(recursive));
    emit logMessage(QString("根目录: %1").arg(rootPath));
    if (recursive)
        emit logMessage("  启用了递归查找，正在遍历子目录...");

    m_records.clear();
    emit recordsChanged();
    emit hasRecordsChanged();

    setIsProcessing(true);
    m_workerRunning = true;
    m_workerThread.start();
}

void BatchRenameController::doWork()
{
    QString rootPath = m_settings->rootPath();
    bool recursive = m_settings->recursive();
    int fileType = m_settings->fileType();
    QString customExtension = m_settings->customExtension();
    int renameMode = m_settings->renameMode();
    QString baseName = m_settings->baseName();
    QString searchText = m_settings->searchText();
    QString replaceText = m_settings->replaceText();

    int successCount = 0, failCount = 0;
    QList<RenameRecord> records;

    auto addRec = [&](const QString &orig, const QString &newP, bool ok, const QString &st) {
        RenameRecord rec;
        rec.originalPath = orig;
        rec.newPath = newP;
        rec.originalName = QFileInfo(orig).fileName();
        rec.newName = QFileInfo(newP).fileName();
        rec.fileType = getFileType(rec.originalName);
        rec.success = ok;
        rec.status = st;
        records.append(rec);
    };

    std::function<void(const QString &)> procDir;
    procDir = [&](const QString &dirPath) {
        QDir currentDir(dirPath);

        QFileInfoList entries = currentDir.entryInfoList(
            QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name);

        QList<QFileInfo> matchingFiles;
        for (const QFileInfo &entry : entries) {
            if (matchesFileType(entry.fileName(), fileType, customExtension))
                matchingFiles.append(entry);
        }

        QMap<QString, QList<QPair<QFileInfo, QString>>> nameGroups;
        for (int i = 0; i < matchingFiles.size(); ++i) {
            const QFileInfo &entry = matchingFiles[i];
            QString extension = getFileExtension(entry.fileName());
            QString nName = generateNewName(i + 1, entry.fileName(), extension,
                                            renameMode, baseName, searchText, replaceText);
            nameGroups[nName].append(qMakePair(entry, nName));
        }

        QMap<QString, QString> finalNames;
        for (auto it = nameGroups.begin(); it != nameGroups.end(); ++it) {
            const QString &baseNameKey = it.key();
            const QList<QPair<QFileInfo, QString>> &group = it.value();

            if (group.size() == 1) {
                QString newPath = currentDir.absoluteFilePath(baseNameKey);
                if (newPath != group.first().first.absoluteFilePath())
                    finalNames[group.first().first.absoluteFilePath()] = baseNameKey;
            } else {
                QString base = QFileInfo(baseNameKey).baseName();
                QString ext = QFileInfo(baseNameKey).suffix();
                for (int i = 0; i < group.size(); ++i) {
                    QString finalName = (i == 0) ? baseNameKey : QString("%1(%2).%3").arg(base).arg(i).arg(ext);
                    QString originalPath = group[i].first.absoluteFilePath();
                    QString newPath = currentDir.absoluteFilePath(finalName);
                    if (newPath != originalPath)
                        finalNames[originalPath] = finalName;
                }
            }
        }

        for (auto it = finalNames.begin(); it != finalNames.end(); ++it) {
            QString originalPath = it.key();
            QString newName = it.value();
            QString newPath = currentDir.absoluteFilePath(newName);
            QString originalFileName = QFileInfo(originalPath).fileName();

            if (QDir(currentDir).rename(originalFileName, newName)) {
                addRec(originalPath, newPath, true, QStringLiteral("已重命名"));
                successCount++;
                emit logMessage(QString("  [重命名] %1 → %2").arg(originalFileName, newName));
            } else {
                addRec(originalPath, newPath, false, QStringLiteral("失败：重命名失败"));
                failCount++;
                emit logMessage(QString("  [失败] %1 → %2").arg(originalFileName, newName));
            }
        }

        if (recursive) {
            QFileInfoList dirs = currentDir.entryInfoList(
                QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name);
            for (const QFileInfo &dirEntry : dirs)
                procDir(dirEntry.absoluteFilePath());
        }
    };

    procDir(rootPath);

    m_records = records;
    m_workerThread.quit();
    QMetaObject::invokeMethod(this, [this, successCount, failCount]() {
        if (successCount == 0 && failCount == 0) {
            setStatusMessage(QStringLiteral("没有找到匹配的文件"));
            m_logger->info("重命名完成: 没有找到匹配的文件");
        } else if (failCount == 0) {
            setStatusMessage(QStringLiteral("成功重命名 %1 个文件").arg(successCount));
            m_logger->info(QString("重命名完成: 成功 %1 个").arg(successCount));
        } else {
            setStatusMessage(QStringLiteral("成功 %1 个，失败 %2 个").arg(successCount).arg(failCount));
            m_logger->info(QString("重命名完成: 成功 %1 个，失败 %2 个").arg(successCount).arg(failCount));
        }
        emit recordsChanged();
        emit hasRecordsChanged();
        setIsProcessing(false);
    }, Qt::QueuedConnection);
}

void BatchRenameController::clearRecords()
{
    m_records.clear();
    emit recordsChanged();
    emit hasRecordsChanged();
    setStatusMessage(QString());
}

void BatchRenameController::reset()
{
    cancel();
    m_records.clear();
    setStatusMessage(QString());

    emit recordsChanged();
    emit hasRecordsChanged();
}

void BatchRenameController::restoreRecord(int index)
{
    if (index < 0 || index >= m_records.size()) {
        return;
    }

    RenameRecord &record = m_records[index];
    if (!record.success) {
        return;
    }

    QFileInfo newPathInfo(record.newPath);
    QDir parentDir = newPathInfo.absoluteDir();

    if (!QFileInfo::exists(record.newPath)) {
        record.status = QStringLiteral("失败：文件不存在");
        record.success = false;
        setStatusMessage(QStringLiteral("还原失败：文件不存在"));
        m_logger->warn(QString("还原失败: %1 — 文件不存在").arg(record.newPath));
        emit recordsChanged();
        return;
    }

    if (QFileInfo::exists(record.originalPath)) {
        record.status = QStringLiteral("失败：原文件已存在");
        record.success = false;
        setStatusMessage(QStringLiteral("还原失败：原文件已存在"));
        m_logger->warn(QString("还原失败: %1 — 原文件已存在").arg(record.originalPath));
        emit recordsChanged();
        return;
    }

    if (parentDir.rename(newPathInfo.fileName(), QFileInfo(record.originalPath).fileName())) {
        record.status = QStringLiteral("已还原");
        record.success = false;
        setStatusMessage(QStringLiteral("已还原：%1").arg(record.originalName));
        m_logger->info(QString("已还原: %1 → %2").arg(record.newName, record.originalName));
    } else {
        record.status = QStringLiteral("失败：还原失败");
        setStatusMessage(QStringLiteral("还原失败：%1").arg(record.newName));
        m_logger->error(QString("还原失败: %1").arg(record.newName));
    }

    emit recordsChanged();
}

void BatchRenameController::restoreAllRecords()
{
    m_logger->info("===== 开始批量还原 =====");
    int successCount = 0;
    int failCount = 0;

    for (int i = 0; i < m_records.size(); ++i) {
        RenameRecord &record = m_records[i];
        if (!record.success) {
            continue;
        }

        QFileInfo newPathInfo(record.newPath);
        QDir parentDir = newPathInfo.absoluteDir();

        if (!QFileInfo::exists(record.newPath)) {
            record.status = QStringLiteral("失败：文件不存在");
            record.success = false;
            failCount++;
            continue;
        }

        if (QFileInfo::exists(record.originalPath)) {
            record.status = QStringLiteral("失败：原文件已存在");
            record.success = false;
            failCount++;
            continue;
        }

        if (parentDir.rename(newPathInfo.fileName(), QFileInfo(record.originalPath).fileName())) {
            record.status = QStringLiteral("已还原");
            record.success = false;
            successCount++;
        } else {
            record.status = QStringLiteral("失败：还原失败");
            failCount++;
        }
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

    emit recordsChanged();
}

void BatchRenameController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}

bool BatchRenameController::isProcessing() const
{
    return m_isProcessing;
}

void BatchRenameController::cancel()
{
    if (m_workerRunning) {
        m_workerThread.quit();
        if (!m_workerThread.wait(3000)) {
            m_workerThread.terminate();
            m_workerThread.wait(3000);
        }
    }
    if (m_isProcessing) {
        m_logger->info("批量重命名已取消");
        setIsProcessing(false);
        setStatusMessage("已取消");
    }
}

void BatchRenameController::setIsProcessing(bool processing)
{
    if (m_isProcessing == processing) return;
    m_isProcessing = processing;
    emit isProcessingChanged();
}

void BatchRenameController::addRecord(const QString &originalPath, const QString &newPath, bool success, const QString &status)
{
    RenameRecord record;
    record.originalPath = originalPath;
    record.newPath = newPath;
    record.originalName = QFileInfo(originalPath).fileName();
    record.newName = QFileInfo(newPath).fileName();
    record.fileType = getFileType(record.originalName);
    record.success = success;
    record.status = status;
    m_records.append(record);
}

QString BatchRenameController::getFileType(const QString &fileName) const
{
    QString ext = getFileExtension(fileName).toLower();
    
    if (m_videoExtensions.contains(ext)) {
        return QString("视频(%1)").arg(ext.mid(1));
    } else if (m_audioExtensions.contains(ext)) {
        return QString("音频(%1)").arg(ext.mid(1));
    } else if (m_imageExtensions.contains(ext)) {
        return QString("图片(%1)").arg(ext.mid(1));
    } else if (m_textExtensions.contains(ext)) {
        return QString("文本(%1)").arg(ext.mid(1));
    } else {
        return QString("其他(%1)").arg(ext.mid(1));
    }
}

bool BatchRenameController::matchesFileType(const QString &fileName, int fileType, const QString &customExtension) const
{
    QString ext = getFileExtension(fileName).toLower();

    switch (fileType) {
    case 0:
        return true;
    case 1:
        return m_videoExtensions.contains(ext);
    case 2:
        return m_audioExtensions.contains(ext);
    case 3:
        return m_textExtensions.contains(ext);
    case 4:
        return m_imageExtensions.contains(ext);
    case 5: {
        if (customExtension.isEmpty()) {
            return true;
        }
        QString customExt = customExtension.toLower();
        if (!customExt.startsWith('.')) {
            customExt = '.' + customExt;
        }
        return ext == customExt;
    }
    default:
        return true;
    }
}

QString BatchRenameController::generateNewName(int index, const QString &originalName, const QString &extension,
                                                int renameMode, const QString &baseName,
                                                const QString &searchText, const QString &replaceText) const
{
    if (renameMode == 0) {
        return QString("%1%2").arg(baseName).arg(extension);
    } else {
        QString nameWithoutExt = originalName;
        if (!extension.isEmpty()) {
            nameWithoutExt = originalName.left(originalName.length() - extension.length());
        }
        QString newName = nameWithoutExt;
        if (!searchText.isEmpty()) {
            newName = newName.replace(searchText, replaceText);
        }
        return newName + extension;
    }
}

QString BatchRenameController::getFileExtension(const QString &fileName) const
{
    int dotIndex = fileName.lastIndexOf('.');
    if (dotIndex > 0 && dotIndex < fileName.length() - 1) {
        return fileName.mid(dotIndex);
    }
    return QString();
}