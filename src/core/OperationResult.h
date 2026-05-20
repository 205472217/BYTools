#pragma once

#include <QString>

struct OperationResult
{
    bool success = false;
    QString message;

    static OperationResult ok(const QString &message = {})
    {
        return {true, message};
    }

    static OperationResult fail(const QString &message)
    {
        return {false, message};
    }
};
