#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QByteArray>

class PluginLogger;

/// 封装 QProcess + QTimer 空闲超时管理的基类。
/// 子类只需重写虚方法处理自己的业务逻辑（进度解析、完成处理等）。
class ProcessRunner : public QObject
{
    Q_OBJECT
public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner() override;

    /// 返回进程是否正在运行
    bool isRunning() const;

    /// 设置日志记录器（可选，不设置则跳过日志输出）
    void setLogger(PluginLogger *logger);

protected:
    // ── 供子类调用的方法 ────────────────────────────────

    /// 启动进程，自动连接信号槽，启动空闲超时定时器
    /// @param idleTimeoutMs  空闲超时毫秒数
    /// @param graceful       是否支持优雅退出（写 q\n 到 stdin）
    /// @param connectStdout  是否同时监听 stdout（默认仅 stderr）
    void startProcess(const QString &program, const QStringList &args,
                      int idleTimeoutMs = 120000, bool graceful = false,
                      bool connectStdout = false);

    /// 取消当前进程
    void cancelProcess();

    /// 累积的 stderr+stdout 缓冲区（从最近一次 startProcess 算起）
    QByteArray stderrBuffer() const { return m_stderrBuffer; }

    /// 清空缓冲区
    void clearStderrBuffer() { m_stderrBuffer.clear(); }

    // ── 供子类重写的虚方法 ──────────────────────────────

    /// 每次收到进程输出时调用（默认空实现）
    virtual void onStderrData(const QByteArray & /*data*/) {}

    /// 进程正常/异常结束时调用（纯虚——子类必须实现）
    virtual void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) = 0;

    /// 空闲超时时调用（默认：kill 进程并打印日志）
    virtual void onProcessTimeout();

    /// 进程出错时调用（默认：FailedToStart 时打印错误）
    virtual void onProcessError(QProcess::ProcessError error);

    // ── 子类可直接访问的成员 ────────────────────────────
    PluginLogger *m_logger = nullptr;
    QProcess *m_process = nullptr;
    QTimer *m_timer = nullptr;

private slots:
    void handleReadyRead();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleError(QProcess::ProcessError error);
    void handleTimeout();

private:
    QByteArray m_stderrBuffer;
    int m_idleTimeoutMs = 120000;
    bool m_graceful = false;
    bool m_connectStdout = false;
};
