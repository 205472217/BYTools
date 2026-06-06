#pragma once

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QMutex>
#include <QStringConverter>

/**
 * 简易文件日志工具，供 VideoSubtitle 插件使用。
 * 日志输出到 <exe同目录>/log/video-subtitle_YYYY-MM-DD.log
 */

// 配置文件的通用路径（exe 同级 config.ini，[VideoSubtitle] 节）
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
        QString fileName = logDir + "/video-subtitle_"
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

    /** REST 请求日志 */
    static void restRequest(const QString &method, const QString &url, const QString &body = QString())
    {
        log("REST", QString("→ %1 %2").arg(method, url));
        if (!body.isEmpty())
            log("REST", QString("  请求参数: %1").arg(body));
    }

    /** REST 响应日志 */
    static void restResponse(int statusCode, const QString &body)
    {
        log("REST", QString("  响应状态: %1").arg(statusCode));
        log("REST", QString("  响应内容: %1").arg(body));
    }

private:
    inline static QMutex s_mutex;
};
