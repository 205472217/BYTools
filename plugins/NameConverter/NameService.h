#pragma once

#include <QList>
#include <QString>
#include <functional>

#include "OperationResult.h"
#include "NamePreviewModel.h"
#include "FileRenameUtils.h"

class PluginLogger;

struct NameExecutionResult
{
    OperationResult result;
    QList<NamePreviewItem> records;
};

using NameProgressCallback = std::function<void(int index, const NamePreviewItem&)>;

class NameService
{
public:
    enum class TargetType {
        Files,
        Directories,
        FilesAndDirectories
    };

    explicit NameService(PluginLogger *logger = nullptr);

    QList<NamePreviewItem> preview(const QString &rootPath, TargetType targetType, bool recursive = false) const;
    NameExecutionResult execute(const QString &rootPath, TargetType targetType, bool recursive = false,
                                NameProgressCallback onItemProcessed = nullptr) const;
    OperationResult restore(const NamePreviewItem &item) const;

private:
    bool shouldInclude(bool isDirectory, TargetType targetType) const;
    NamePreviewItem makeItem(const QString &absolutePath, bool isDirectory) const;

    PluginLogger *m_logger = nullptr;

};