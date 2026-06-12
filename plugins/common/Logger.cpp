#include "Logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QThread>

PluginLogger::PluginLogger(const QString &prefix)
    : m_prefix(prefix)
{
}

PluginLogger::~PluginLogger() = default;

void PluginLogger::log(const QString &level, const QString &message)
{
    QMutexLocker locker(&m_mutex);

    // <exe_dir>/log/<prefix>_YYYY-MM-DD.log
    const QString logDir = QCoreApplication::applicationDirPath() + QStringLiteral("/log");
    QDir().mkpath(logDir);

    const QString dateStr = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    const QString filePath = logDir + QStringLiteral("/") + m_prefix + QStringLiteral("_")
                             + dateStr + QStringLiteral(".log");

    QFile file(filePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
        const QString tid = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
        out << timestamp << QStringLiteral(" [") << level << QStringLiteral("] [")
            << tid << QStringLiteral("] ") << message << QStringLiteral("\n");
    }
}

void PluginLogger::info(const QString &msg)
{
    log(QStringLiteral("INFO"), msg);
}

void PluginLogger::warn(const QString &msg)
{
    log(QStringLiteral("WARN"), msg);
}

void PluginLogger::error(const QString &msg)
{
    log(QStringLiteral("ERROR"), msg);
}

void PluginLogger::restRequest(const QString &method, const QString &url,
                                const QString &body)
{
    QString msg = method + QStringLiteral(" ") + url;
    if (!body.isEmpty())
        msg += QStringLiteral("\n  body: ") + body;
    log(QStringLiteral("REST-REQ"), msg);
}

void PluginLogger::restResponse(int statusCode, const QString &body)
{
    QString msg = QString::number(statusCode);
    if (!body.isEmpty())
        msg += QStringLiteral("\n  body: ") + body;
    log(QStringLiteral("REST-RES"), msg);
}
