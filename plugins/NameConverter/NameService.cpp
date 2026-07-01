#include "NameService.h"
#include "Config.h"
#include "Logger.h"

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>

namespace {

void replaceRecordPrefixes(QList<NamePreviewItem> &records, const QString &fromPrefix, const QString &toPrefix)
{
    for (auto &record : records) {
        replacePathPrefix(record.currentPath, fromPrefix, toPrefix);
        replacePathPrefix(record.newPath, fromPrefix, toPrefix);
    }
}

}

NameService::NameService(const ITextConverter &converter, PluginLogger *logger)
    : m_logger(logger)
    , m_converter(converter)
{
}

QList<NamePreviewItem> NameService::preview(const QString &rootPath, TargetType targetType, bool recursive) const
{
    QList<NamePreviewItem> items;
    const QDir root(rootPath);
    if (!root.exists()) {
        return items;
    }

    const QFileInfoList entries = root.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
        QDir::DirsFirst | QDir::Name);

    for (const QFileInfo &entry : entries) {
        if (recursive && entry.isDir()) {
            items.append(preview(entry.absoluteFilePath(), targetType, recursive));
        }

        if (shouldInclude(entry.isDir(), targetType)) {
            const auto item = makeItem(entry.absoluteFilePath(), entry.isDir());
            if (item.currentName != item.newName) {
                items.append(item);
            }
        }
    }

    return items;
}

NameExecutionResult NameService::execute(const QString &rootPath, TargetType targetType, bool recursive) const
{
    const QList<NamePreviewItem> items = preview(rootPath, targetType, recursive);
    int successCount = 0;
    QStringList failures;
    QList<NamePreviewItem> records;

    for (const auto &item : items) {
        if (item.currentPath == item.newPath) {
            continue;
        }

        if (QFileInfo::exists(item.newPath)) {
            const QString message = QStringLiteral("%1 已存在").arg(item.newPath);
            auto record = item;
            record.status = QStringLiteral("失败：目标已存在");
            records.append(record);
            failures.append(message);
            m_logger->warn(QString("  [失败] %1 → %2 — 目标已存在")
                .arg(item.currentName, item.newName));
            continue;
        }

        QDir parent(QFileInfo(item.currentPath).absolutePath());
        if (parent.rename(QFileInfo(item.currentPath).fileName(), QFileInfo(item.newPath).fileName())) {
            ++successCount;
            if (item.directory) {
                replaceRecordPrefixes(records, item.currentPath, item.newPath);
            }
            auto record = item;
            record.status = QStringLiteral("已转换");
            records.append(record);
            m_logger->info(QString("  [转换] %1 → %2").arg(item.currentName, item.newName));
        } else {
            const QString message = QStringLiteral("%1 重命名失败").arg(item.currentPath);
            auto record = item;
            record.status = QStringLiteral("失败：重命名失败");
            records.append(record);
            failures.append(message);
            m_logger->error(QString("  [失败] %1 → %2 — 重命名失败")
                .arg(item.currentName, item.newName));
        }
    }

    if (!failures.isEmpty()) {
        return {
            OperationResult::fail(QStringLiteral("已完成 %1 项，失败 %2 项：%3")
                .arg(successCount)
                .arg(failures.count())
                .arg(failures.join(QStringLiteral("；")))),
            records
        };
    }

    return {
        OperationResult::ok(QStringLiteral("已完成 %1 项转换").arg(successCount)),
        records
    };
}

OperationResult NameService::restore(const NamePreviewItem &item) const
{
    if (item.currentPath.isEmpty() || item.newPath.isEmpty()) {
        return OperationResult::fail(QStringLiteral("记录无效，无法还原"));
    }

    if (!QFileInfo::exists(item.newPath)) {
        return OperationResult::fail(QStringLiteral("新路径不存在，无法还原：%1").arg(item.newPath));
    }

    if (QFileInfo::exists(item.currentPath)) {
        return OperationResult::fail(QStringLiteral("原路径已存在，无法还原：%1").arg(item.currentPath));
    }

    QDir parent(QFileInfo(item.newPath).absolutePath());
    if (parent.rename(QFileInfo(item.newPath).fileName(), QFileInfo(item.currentPath).fileName())) {
        return OperationResult::ok(QStringLiteral("已还原：%1").arg(item.currentName));
    }

    return OperationResult::fail(QStringLiteral("还原失败：%1").arg(item.newPath));
}

bool NameService::shouldInclude(bool isDirectory, TargetType targetType) const
{
    switch (targetType) {
    case TargetType::Files:
        return !isDirectory;
    case TargetType::Directories:
        return isDirectory;
    case TargetType::FilesAndDirectories:
        return true;
    }
    return false;
}

NamePreviewItem NameService::makeItem(const QString &absolutePath, bool isDirectory) const
{
    const QFileInfo info(absolutePath);
    const QString newName = m_converter.traditionalToSimplified(info.fileName());
    const QString newPath = QDir(info.absolutePath()).absoluteFilePath(newName);

    return {
        info.absoluteFilePath(),
        newPath,
        info.fileName(),
        newName,
        isDirectory,
        QStringLiteral("待处理")
    };
}