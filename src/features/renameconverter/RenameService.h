#pragma once

#include <QList>
#include <QString>

#include "../../core/OperationResult.h"
#include "RenamePreviewModel.h"
#include "TextConverter.h"

struct RenameExecutionResult
{
    OperationResult result;
    QList<RenamePreviewItem> records;
};

class RenameService
{
public:
    enum class TargetType {
        Files,
        Directories,
        FilesAndDirectories
    };

    explicit RenameService(const ITextConverter &converter);

    QList<RenamePreviewItem> preview(const QString &rootPath, TargetType targetType) const;
    RenameExecutionResult execute(const QString &rootPath, TargetType targetType) const;
    OperationResult restore(const RenamePreviewItem &item) const;

private:
    bool shouldInclude(bool isDirectory, TargetType targetType) const;
    RenamePreviewItem makeItem(const QString &absolutePath, bool isDirectory) const;

    const ITextConverter &m_converter;
};
