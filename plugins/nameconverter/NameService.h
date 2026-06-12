#pragma once

#include <QList>
#include <QString>

#include "OperationResult.h"
#include "NamePreviewModel.h"
#include "TextConverter.h"

class PluginLogger;

struct NameExecutionResult
{
    OperationResult result;
    QList<NamePreviewItem> records;
};

class NameService
{
public:
    enum class TargetType {
        Files,
        Directories,
        FilesAndDirectories
    };

    explicit NameService(const ITextConverter &converter, PluginLogger *logger = nullptr);

    QList<NamePreviewItem> preview(const QString &rootPath, TargetType targetType, bool recursive = false) const;
    NameExecutionResult execute(const QString &rootPath, TargetType targetType, bool recursive = false) const;
    OperationResult restore(const NamePreviewItem &item) const;

private:
    bool shouldInclude(bool isDirectory, TargetType targetType) const;
    NamePreviewItem makeItem(const QString &absolutePath, bool isDirectory) const;

    PluginLogger *m_logger = nullptr;
    const ITextConverter &m_converter;
};