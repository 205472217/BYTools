#pragma once

#include <QString>

class ITextConverter
{
public:
    virtual ~ITextConverter() = default;
    virtual QString traditionalToSimplified(const QString &text) const = 0;
};

class ChineseTextConverter final : public ITextConverter
{
public:
    QString traditionalToSimplified(const QString &text) const override;
};
