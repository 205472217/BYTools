#pragma once

#include <QString>
#include <QDebug>

namespace PluginLogger {

inline void info(const QString &msg)
{
    qDebug().noquote() << "[CustomSubtitle]" << msg;
}

inline void error(const QString &msg)
{
    qDebug().noquote() << "[CustomSubtitle][ERROR]" << msg;
}

} // namespace PluginLogger
