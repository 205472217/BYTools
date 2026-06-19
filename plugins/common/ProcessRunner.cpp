#include "ProcessRunner.h"
#include "Logger.h"

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent)
{
}

ProcessRunner::~ProcessRunner()
{
    cancelProcess();
}

void ProcessRunner::setLogger(PluginLogger *logger)
{
    m_logger = logger;
}

bool ProcessRunner::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void ProcessRunner::startProcess(const QString &program, const QStringList &args,
                                  int idleTimeoutMs, bool graceful,
                                  bool connectStdout)
{
    // 总是清理旧进程（无论是否还在运行），避免在 onProcessFinished 中重启时泄漏
    cancelProcess();

    m_stderrBuffer.clear();
    m_idleTimeoutMs = idleTimeoutMs;
    m_graceful = graceful;
    m_connectStdout = connectStdout;

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &ProcessRunner::handleReadyRead);
    if (connectStdout) {
        connect(m_process, &QProcess::readyReadStandardOutput,
                this, &ProcessRunner::handleReadyRead);
    }
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessRunner::handleFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &ProcessRunner::handleError);

    if (!m_timer) {
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        connect(m_timer, &QTimer::timeout, this, &ProcessRunner::handleTimeout);
    }

    m_process->start(program, args);
    m_timer->start(m_idleTimeoutMs);
}

void ProcessRunner::cancelProcess()
{
    if (m_timer)
        m_timer->stop();

    if (m_process && m_process->state() != QProcess::NotRunning) {
        if (m_graceful) {
            m_process->write("q\n");
            if (!m_process->waitForFinished(15000)) {
                m_process->kill();
                m_process->waitForFinished(3000);
            }
        } else {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
    }

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

// ── Private slots ────────────────────────────────────────

void ProcessRunner::handleReadyRead()
{
    if (!m_process) return;

    // 空闲超时重置
    if (m_timer)
        m_timer->start(m_idleTimeoutMs);

    // 累积输出（限制 64KB）
    QByteArray chunk = m_process->readAllStandardError();
    if (m_connectStdout)
        chunk += m_process->readAllStandardOutput();
    m_stderrBuffer += chunk;
    if (m_stderrBuffer.size() > 65536)
        m_stderrBuffer = m_stderrBuffer.right(65536);

    onStderrData(chunk);
}

void ProcessRunner::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_timer)
        m_timer->stop();
    onProcessFinished(exitCode, exitStatus);
    // 若子类在 onProcessFinished 中没有重启新进程，则自动清理
    if (m_process && m_process->state() == QProcess::NotRunning) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void ProcessRunner::handleError(QProcess::ProcessError error)
{
    if (m_timer)
        m_timer->stop();
    onProcessError(error);
    // 若子类没有重启新进程，则自动清理
    if (m_process && m_process->state() == QProcess::NotRunning) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void ProcessRunner::handleTimeout()
{
    onProcessTimeout();
}

// ── Default virtual implementations ──────────────────────

void ProcessRunner::onProcessTimeout()
{
    if (m_logger)
        m_logger->error("ProcessRunner: 进程空闲超时，强制终止");
    cancelProcess();
}

void ProcessRunner::onProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart && m_logger)
        m_logger->error("ProcessRunner: 进程启动失败");
}
