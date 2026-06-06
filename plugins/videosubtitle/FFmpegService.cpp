#include "FFmpegService.h"
#include "PluginLogger.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <QDir>

// Helper: convert HTML color #RRGGBB to ASS color &H00BBGGRR
static QString htmlColorToAss(const QString &htmlColor)
{
    QString c = htmlColor.trimmed();
    if (c.startsWith('#'))
        c = c.mid(1);
    if (c.length() == 6) {
        // RRGGBB → AABBGGRR (AA=00 fully opaque)
        return QString("&H00%1%2%3")
            .arg(c.mid(4, 2))  // BB
            .arg(c.mid(2, 2))  // GG
            .arg(c.mid(0, 2)); // RR
    }
    return "&H00FFFFFF";
}

FFmpegService::FFmpegService(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_timer(new QTimer(this))
    , m_isExtracting(false)
    , m_totalDuration(0)
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &FFmpegService::onProcessTimeout);
}

void FFmpegService::startExtractAudio(const QString &ffmpegPath,
                                       const QString &videoPath,
                                       const QString &outputWav)
{
    if (m_process) {
        cancel();
    }

    m_outputPath = outputWav;
    m_isExtracting = true;

    // Get total duration for progress
    m_totalDuration = getVideoDuration(ffmpegPath, videoPath);

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &FFmpegService::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FFmpegService::onProcessFinished);

    QStringList args;
    args << "-i" << videoPath
         << "-vn"
         << "-acodec" << "pcm_s16le"
         << "-ar" << "16000"
         << "-ac" << "1"
         << "-y"
         << outputWav;

    m_process->start(ffmpegPath, args);

    // 空闲超时: 只要 ffmpeg 还在输出进度就是活着的，计时器不断重置
    m_timer->start(120000);
    PluginLogger::info("FFmpeg 音频提取空闲超时: 120 秒（有进度输出自动续期）");
}

void FFmpegService::startBurnSubtitles(const QString &ffmpegPath,
                                        const QString &videoPath,
                                        const QString &srtPath,
                                        const QString &outputPath,
                                        int fontSize,
                                        const QString &fontColor,
                                        const QString &borderColor,
                                        int borderWidth)
{
    if (m_process) {
        cancel();
    }

    m_outputPath = outputPath;
    m_isExtracting = false;

    m_totalDuration = getVideoDuration(ffmpegPath, videoPath);

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &FFmpegService::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FFmpegService::onProcessFinished);

    // Build subtitle filter with full style
    QString escapedSrtPath = srtPath;
    escapedSrtPath.replace("\\", "/");
    escapedSrtPath.replace(":", "\\:");
    QString assFontColor = htmlColorToAss(fontColor);
    QString assBorderColor = htmlColorToAss(borderColor);
    QString styleFilter =
        QString("subtitles='%1':force_style='FontSize=%2,PrimaryColour=%3,OutlineColour=%4,BorderStyle=1,Outline=%5,Shadow=1'")
            .arg(escapedSrtPath,
                 QString::number(fontSize),
                 assFontColor,
                 assBorderColor,
                 QString::number(borderWidth));

    QStringList args;
    args << "-i" << videoPath
         << "-vf" << styleFilter
         << "-c:a" << "copy"
         << "-y"
         << outputPath;

    m_process->start(ffmpegPath, args);

    // 空闲超时: 只要 ffmpeg 还在输出进度就是活着的，计时器不断重置
    m_timer->start(120000);
    PluginLogger::info("FFmpeg 烧录空闲超时: 120 秒（有进度输出自动续期）");
}

void FFmpegService::cancel()
{
    m_timer->stop();
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

bool FFmpegService::isFFmpegAvailable(const QString &ffmpegPath)
{
    if (ffmpegPath.isEmpty()) return false;

    // 在 Windows 上使用原生反斜杠路径，避免 QProcess 解析问题
    QString nativePath = QDir::toNativeSeparators(ffmpegPath);

    QProcess proc;
    proc.start(nativePath, {"-version"});

    // 检查是否成功启动
    if (!proc.waitForStarted(3000)) {
        PluginLogger::warn(QString("FFmpeg 启动失败: %1, 错误: %2")
            .arg(nativePath, proc.errorString()));
        return false;
    }

    if (!proc.waitForFinished(5000)) {
        // 超时，强制终止
        proc.kill();
        proc.waitForFinished(2000);
        PluginLogger::warn(QString("FFmpeg 执行超时: %1").arg(nativePath));
        return false;
    }

    bool success = (proc.exitCode() == 0);
    if (!success) {
        PluginLogger::warn(QString("FFmpeg 返回值非零: %1, exitCode=%2, stderr=%3")
            .arg(nativePath)
            .arg(proc.exitCode())
            .arg(QString::fromUtf8(proc.readAllStandardError()).trimmed()));
    }
    return success;
}

QString FFmpegService::ffmpegVersion(const QString &ffmpegPath)
{
    if (ffmpegPath.isEmpty()) return QString();

    QString nativePath = QDir::toNativeSeparators(ffmpegPath);

    QProcess proc;
    proc.start(nativePath, {"-version"});

    if (!proc.waitForStarted(3000) ||
        !proc.waitForFinished(5000) ||
        proc.exitCode() != 0) {
        return QString();
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QRegularExpression re(R"(ffmpeg version (\S+))");
    QRegularExpressionMatch match = re.match(output);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return QString();
}

qint64 FFmpegService::getVideoDuration(const QString &ffmpegPath, const QString &videoPath)
{
    if (ffmpegPath.isEmpty() || videoPath.isEmpty()) return 0;

    QProcess proc;
    proc.start(ffmpegPath, {"-i", videoPath});
    proc.waitForFinished(10000);

    QString output = QString::fromUtf8(proc.readAllStandardError());
    QRegularExpression re(R"(Duration:\s*(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    QRegularExpressionMatch match = re.match(output);
    if (!match.hasMatch()) return 0;

    qint64 hours = match.captured(1).toLongLong();
    qint64 minutes = match.captured(2).toLongLong();
    qint64 seconds = match.captured(3).toLongLong();
    qint64 centiseconds = match.captured(4).toLongLong();

    return (hours * 3600 + minutes * 60 + seconds) * 1000 + centiseconds * 10;
}

void FFmpegService::onProcessReadyRead()
{
    if (!m_process) return;

    // 收到进程输出 → 重置空闲超时（ffmpeg 还在工作）
    m_timer->start(120000);

    QString output = QString::fromUtf8(m_process->readAllStandardError());

    // Parse progress from FFmpeg output
    // Look for "time=HH:MM:SS.ss" or "size=xxxkB"
    QRegularExpression timeRe(R"(time=(\d{2}):(\d{2}):(\d{2})\.(\d{2}))");
    QRegularExpressionMatch match = timeRe.match(output);
    if (match.hasMatch() && m_totalDuration > 0) {
        qint64 hours = match.captured(1).toLongLong();
        qint64 minutes = match.captured(2).toLongLong();
        qint64 seconds = match.captured(3).toLongLong();
        qint64 centiseconds = match.captured(4).toLongLong();

        qint64 currentTime = (hours * 3600 + minutes * 60 + seconds) * 1000 + centiseconds * 10;
        double progressValue = qMin(1.0, static_cast<double>(currentTime) / m_totalDuration);
        emit progress(progressValue);
    }
}

void FFmpegService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_timer->stop();
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    QString error;

    if (!success) {
        error = QString::fromUtf8(m_process->readAllStandardError());
    }

    m_process->deleteLater();
    m_process = nullptr;

    emit progress(1.0);
    emit finished(success, m_outputPath, error);
}

void FFmpegService::onProcessTimeout()
{
    PluginLogger::error(QString("FFmpeg 进程超时，强制终止: %1").arg(m_outputPath));

    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
        m_process->deleteLater();
        m_process = nullptr;
    }

    emit progress(1.0);
    emit finished(false, m_outputPath, QString("%1 超时，进程已终止")
        .arg(m_isExtracting ? "音频提取" : "烧录字幕"));
}
