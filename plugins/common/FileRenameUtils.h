#pragma once

#include <QString>
#include <QDir>

QString traditionalToSimplified(const QString &text);
QString getFileExtension(const QString &fileName);

bool renameFile(const QString &oldPath, const QString &newPath);
