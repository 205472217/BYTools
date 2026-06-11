#pragma once

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QMutex>
#include <QStringConverter>

/**
 * 文件日志工具，供 CustomSubtitle 插件使用。
 * 日志输出到 <exe同目录>/log/custom-subtitle_YYYY-MM-DD.log
 */

// 配置文件的通用路径（exe 同级 config.ini，[customsubtitle] 节）
inline QString pluginConfigFilePath()
{
    return QCoreApplication::applicationDirPath() + "/config.ini";
}

class PluginLogger
{
public:
    static void log(const QString &level, const QString &message)
    {
        QMutexLocker locker(&s_mutex);

        QString logDir = QCoreApplication::applicationDirPath() + "/log";
        QDir().mkpath(logDir);
        QString fileName = logDir + "/custom-subtitle_"
                           + QDateTime::currentDateTime().toString("yyyy-MM-dd")
                           + ".log";

        QFile file(fileName);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "] "
                << "[" << level << "] "
                << message << "\n";
        }
    }

    static void info(const QString &msg)  { log("INFO", msg); }
    static void warn(const QString &msg)  { log("WARN", msg); }
    static void error(const QString &msg) { log("ERROR", msg); }

private:
    inline static QMutex s_mutex;
};
