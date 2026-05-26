#pragma once

#include <QList>
#include <QString>

#include "OperationResult.h"
#include "NamePreviewModel.h"
#include "TextConverter.h"

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

    explicit NameService(const ITextConverter &converter);

    QList<NamePreviewItem> preview(const QString &rootPath, TargetType targetType, bool recursive = false) const;
    NameExecutionResult execute(const QString &rootPath, TargetType targetType, bool recursive = false) const;
    OperationResult restore(const NamePreviewItem &item) const;

private:
    bool shouldInclude(bool isDirectory, TargetType targetType) const;
    NamePreviewItem makeItem(const QString &absolutePath, bool isDirectory) const;

    const ITextConverter &m_converter;
};