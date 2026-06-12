#pragma once

#include <QString>
#include <QMutex>
#include <QFile>
#include <QDate>

/**
 * 日志工具类，每个插件持有独立实例。
 * 例：PluginLogger logger("custom-subtitle")
 *     → 日志输出到 <exe_dir>/log/custom-subtitle_YYYY-MM-DD.log
 */
class PluginLogger
{
public:
    explicit PluginLogger(const QString &prefix);
    ~PluginLogger();

    // 禁止拷贝
    PluginLogger(const PluginLogger &) = delete;
    PluginLogger &operator=(const PluginLogger &) = delete;

    void info(const QString &msg);
    void warn(const QString &msg);
    void error(const QString &msg);

    // REST 请求/响应日志（videosubtitle 插件使用）
    void restRequest(const QString &method, const QString &url,
                     const QString &body = QString());
    void restResponse(int statusCode, const QString &body);

private:
    void log(const QString &level, const QString &message);

    // 确保文件已打开且日期正确，跨日自动轮转
    void ensureFileOpen();
    void closeFile();

    QString m_prefix;
    QMutex m_mutex;

    QFile *m_logFile = nullptr;     // 持久打开的日志文件句柄
    QDate  m_currentDate;           // 当前写入的日期，用于检测跨日
};
