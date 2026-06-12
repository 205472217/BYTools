#include "Logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

PluginLogger::PluginLogger(const QString &prefix)
    : m_prefix(prefix)
{
    // 启动时立即打开日志文件，确保文件被独占锁定
    ensureFileOpen();
}

PluginLogger::~PluginLogger()
{
    QMutexLocker locker(&m_mutex);
    closeFile();
}

void PluginLogger::log(const QString &level, const QString &message)
{
    QMutexLocker locker(&m_mutex);

    // 跨日自动轮转：日期变更则关闭旧文件，打开新文件
    ensureFileOpen();

    if (m_logFile && m_logFile->isOpen()) {
        QTextStream out(m_logFile);
        const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
        const QString tid = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
        out << timestamp << QStringLiteral(" [") << level << QStringLiteral("] [")
            << tid << QStringLiteral("] ") << message << QStringLiteral("\n");
        out.flush();  // 确保实时落盘，不依赖 QTextStream 缓冲
    }
}

void PluginLogger::ensureFileOpen()
{
    QDate today = QDate::currentDate();

    // 快速路径：文件已打开且日期未变，无需操作
    if (m_logFile && m_logFile->isOpen() && m_currentDate == today)
        return;

    // 日期变更或初次打开 → 关闭旧文件，打开新文件
    closeFile();

    const QString logDir = QCoreApplication::applicationDirPath() + QStringLiteral("/log");
    QDir().mkpath(logDir);

    const QString dateStr = today.toString(QStringLiteral("yyyy-MM-dd"));
    const QString filePath = logDir + QStringLiteral("/") + m_prefix + QStringLiteral("_")
                             + dateStr + QStringLiteral(".log");

#ifdef Q_OS_WIN
    // Windows: 用 CreateFileW 独占打开，不设 FILE_SHARE_DELETE
    // 这样运行时其他进程无法删除或重命名该日志文件
    HANDLE hFile = CreateFileW(
        (LPCWSTR)filePath.utf16(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,  // 允许读写，禁止删除
        NULL,
        OPEN_ALWAYS,                         // 不存在则创建，存在则打开
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile != INVALID_HANDLE_VALUE) {
        // 定位到文件末尾，实现追加写入
        SetFilePointer(hFile, 0, NULL, FILE_END);

        // 转换 HANDLE → CRT fd → FILE* → QFile
        int fd = _open_osfhandle((intptr_t)hFile, _O_WRONLY | _O_APPEND | _O_TEXT);
        if (fd != -1) {
            FILE *fp = _fdopen(fd, "a");
            if (fp) {
                m_logFile = new QFile();
                m_logFile->open(fp, QIODevice::Append | QIODevice::Text,
                                QFileDevice::AutoCloseHandle);
            } else {
                _close(fd);
            }
        } else {
            CloseHandle(hFile);
        }
    }
#else
    // 非 Windows 平台：至少保持文件持久打开
    m_logFile = new QFile(filePath);
    m_logFile->open(QIODevice::Append | QIODevice::Text);
#endif

    if (m_logFile && m_logFile->isOpen())
        m_currentDate = today;
}

void PluginLogger::closeFile()
{
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
    m_currentDate = QDate();
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
