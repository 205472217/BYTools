#include "BatchRenameController.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QRegularExpression>

BatchRenameController::BatchRenameController(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
{
    updateFileTips();
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

int BatchRenameController::fileType() const
{
    return m_fileType;
}

void BatchRenameController::setFileType(int fileType)
{
    if (m_fileType == fileType) {
        return;
    }

    m_fileType = fileType;
    emit fileTypeChanged();
    updateFileTips();
}

QString BatchRenameController::customExtension() const
{
    return m_customExtension;
}

void BatchRenameController::setCustomExtension(const QString &customExtension)
{
    if (m_customExtension == customExtension) {
        return;
    }

    m_customExtension = customExtension;
    emit customExtensionChanged();
}

QString BatchRenameController::fileTips() const
{
    return m_fileTips;
}

int BatchRenameController::renameMode() const
{
    return m_renameMode;
}

void BatchRenameController::setRenameMode(int renameMode)
{
    if (m_renameMode == renameMode) {
        return;
    }

    m_renameMode = renameMode;
    emit renameModeChanged();
}

QString BatchRenameController::baseName() const
{
    return m_baseName;
}

void BatchRenameController::setBaseName(const QString &baseName)
{
    if (m_baseName == baseName) {
        return;
    }

    m_baseName = baseName;
    emit baseNameChanged();
}

QString BatchRenameController::searchText() const
{
    return m_searchText;
}

void BatchRenameController::setSearchText(const QString &searchText)
{
    if (m_searchText == searchText) {
        return;
    }

    m_searchText = searchText;
    emit searchTextChanged();
}

QString BatchRenameController::replaceText() const
{
    return m_replaceText;
}

void BatchRenameController::setReplaceText(const QString &replaceText)
{
    if (m_replaceText == replaceText) {
        return;
    }

    m_replaceText = replaceText;
    emit replaceTextChanged();
}

bool BatchRenameController::recursive() const
{
    return m_recursive;
}

void BatchRenameController::setRecursive(bool recursive)
{
    if (m_recursive == recursive) {
        return;
    }

    m_recursive = recursive;
    emit recursiveChanged();
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
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        setStatusMessage(QStringLiteral("请选择有效的根文件夹"));
        return;
    }

    m_logger->info(QString("===== 开始批量重命名 ====="));
    m_logger->info(QString("根目录: %1, 递归=%2").arg(m_rootPath).arg(m_recursive));

    m_records.clear();
    emit recordsChanged();
    emit hasRecordsChanged();

    setIsProcessing(true);

    int successCount = 0;
    int failCount = 0;

    processDirectory(QDir(m_rootPath), successCount, failCount);

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
}

void BatchRenameController::processDirectory(const QDir &currentDir, int &successCount, int &failCount)
{
    m_logger->info(QString("处理目录: %1").arg(currentDir.absolutePath()));
    QFileInfoList entries = currentDir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
        QDir::Name);

    QList<QFileInfo> matchingFiles;
    for (const QFileInfo &entry : entries) {
        if (matchesFileType(entry.fileName())) {
            matchingFiles.append(entry);
        }
    }

    QMap<QString, QList<QPair<QFileInfo, QString>>> nameGroups;
    
    for (int i = 0; i < matchingFiles.size(); ++i) {
        const QFileInfo &entry = matchingFiles[i];
        QString extension = getFileExtension(entry.fileName());
        QString newName = generateNewName(i + 1, entry.fileName(), extension);
        nameGroups[newName].append(qMakePair(entry, newName));
    }

    QMap<QString, QString> finalNames;
    
    for (auto it = nameGroups.begin(); it != nameGroups.end(); ++it) {
        const QString &baseName = it.key();
        const QList<QPair<QFileInfo, QString>> &group = it.value();
        
        if (group.size() == 1) {
            QString newPath = currentDir.absoluteFilePath(baseName);
            if (newPath != group.first().first.absoluteFilePath()) {
                finalNames[group.first().first.absoluteFilePath()] = baseName;
            }
        } else {
            QString base = QFileInfo(baseName).baseName();
            QString ext = QFileInfo(baseName).suffix();
            
            for (int i = 0; i < group.size(); ++i) {
                QString finalName;
                if (i == 0) {
                    finalName = baseName;
                } else {
                    finalName = QString("%1(%2).%3").arg(base).arg(i).arg(ext);
                }
                
                QString originalPath = group[i].first.absoluteFilePath();
                QString newPath = currentDir.absoluteFilePath(finalName);
                
                if (newPath != originalPath) {
                    finalNames[originalPath] = finalName;
                }
            }
        }
    }

    for (auto it = finalNames.begin(); it != finalNames.end(); ++it) {
        QString originalPath = it.key();
        QString newName = it.value();
        QString newPath = currentDir.absoluteFilePath(newName);

        QString originalFileName = QFileInfo(originalPath).fileName();

        bool success = false;
        QString status;

        if (QDir(currentDir).rename(originalFileName, newName)) {
            success = true;
            status = QStringLiteral("已重命名");
            successCount++;
            m_logger->info(QString("  [重命名] %1 → %2").arg(originalFileName, newName));
        } else {
            status = QStringLiteral("失败：重命名失败");
            failCount++;
            m_logger->error(QString("  [失败] %1 → %2").arg(originalFileName, newName));
        }

        addRecord(originalPath, newPath, success, status);
    }

    if (m_recursive) {
        QFileInfoList dirs = currentDir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden,
            QDir::Name);

        for (const QFileInfo &dirEntry : dirs) {
            processDirectory(QDir(dirEntry.absoluteFilePath()), successCount, failCount);
        }
    }
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
    m_rootPath.clear();
    m_fileType = 0;
    m_customExtension.clear();
    m_renameMode = 0;
    m_baseName.clear();
    m_searchText.clear();
    m_replaceText.clear();
    m_recursive = false;
    setStatusMessage(QString());
    
    emit recordsChanged();
    emit hasRecordsChanged();
    emit rootPathChanged();
    emit fileTypeChanged();
    emit customExtensionChanged();
    emit renameModeChanged();
    emit baseNameChanged();
    emit searchTextChanged();
    emit replaceTextChanged();
    emit recursiveChanged();
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

void BatchRenameController::updateFileTips()
{
    switch (m_fileType) {
    case 1:
        m_fileTips = QStringLiteral("支持: %1").arg(m_videoExtensions.join(", "));
        break;
    case 2:
        m_fileTips = QStringLiteral("支持: %1").arg(m_audioExtensions.join(", "));
        break;
    case 3:
        m_fileTips = QStringLiteral("支持: %1").arg(m_textExtensions.join(", "));
        break;
    case 4:
        m_fileTips = QStringLiteral("支持: %1").arg(m_imageExtensions.join(", "));
        break;
    default:
        m_fileTips = QString();
        break;
    }
    emit fileTipsChanged();
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

bool BatchRenameController::matchesFileType(const QString &fileName) const
{
    QString ext = getFileExtension(fileName).toLower();

    switch (m_fileType) {
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
        if (m_customExtension.isEmpty()) {
            return true;
        }
        QString customExt = m_customExtension.toLower();
        if (!customExt.startsWith('.')) {
            customExt = '.' + customExt;
        }
        return ext == customExt;
    }
    default:
        return true;
    }
}

QString BatchRenameController::generateNewName(int index, const QString &originalName, const QString &extension) const
{
    if (m_renameMode == 0) {
        QString base = m_baseName.isEmpty() ? QStringLiteral("file") : m_baseName;
        return QString("%1%2").arg(base).arg(extension);
    } else {
        QString nameWithoutExt = originalName;
        if (!extension.isEmpty()) {
            nameWithoutExt = originalName.left(originalName.length() - extension.length());
        }
        QString newName = nameWithoutExt;
        if (!m_searchText.isEmpty()) {
            newName = newName.replace(m_searchText, m_replaceText);
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