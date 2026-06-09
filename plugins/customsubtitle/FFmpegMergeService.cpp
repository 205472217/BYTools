#include "FFmpegMergeService.h"
#include "PluginLogger.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>

FFmpegMergeService::FFmpegMergeService(QObject *parent)
    : QObject(parent)
{
}

bool FFmpegMergeService::isFFmpegAvailable(const QString &ffmpegPath)
{
    if (ffmpegPath.isEmpty()) return false;
    QProcess proc;
    proc.start(ffmpegPath, {"-version"});
    proc.waitForFinished(5000);
    return proc.exitCode() == 0;
}

void FFmpegMergeService::startMerge(const QString &ffmpegPath,
                                     const QString &videoDir,
                                     const QString &outputDir,
                                     bool recursive,
                                     bool useGpu)
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        emit finished(false, "已有任务正在执行");
        return;
    }

    m_ffmpegPath = ffmpegPath;
    m_outputDir = outputDir;
    m_currentIndex = -1;
    m_cancelled = false;
    m_successCount = 0;
    m_failCount = 0;
    m_pendingFiles.clear();

    QDir outDir(outputDir);
    if (!outDir.exists()) {
        outDir.mkpath(".");
    }

    // Scan video directory for videos with matching subtitle files
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) flags = QDirIterator::Subdirectories;

    QDirIterator it(videoDir, QDir::Files, flags);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
        QString lower = fi.suffix().toLower();
        if (!m_videoExts.contains("." + lower)) continue;

        // Look for matching subtitle file
        QString basePath = fi.absolutePath() + "/" + fi.completeBaseName();
        QString subPath;
        for (const QString &ext : m_subtitleExts) {
            QString candidate = basePath + ext;
            if (QFileInfo::exists(candidate)) {
                subPath = candidate;
                break;
            }
        }
        if (subPath.isEmpty()) continue; // no matching subtitle, skip

        VideoFile vf;
        vf.path = fi.absoluteFilePath();
        vf.subtitlePath = subPath;
        vf.outputPath = outputDir + "/" + fi.fileName();

        // Handle duplicate output names
        if (QFileInfo::exists(vf.outputPath)) {
            int idx = 2;
            QString stem = fi.completeBaseName();
            QString ext = fi.suffix();
            while (QFileInfo::exists(outputDir + "/" + stem + "_副本" + QString::number(idx) + "." + ext)) {
                idx++;
            }
            vf.outputPath = outputDir + "/" + stem + "_副本" + QString::number(idx) + "." + ext;
        }

        m_pendingFiles.append(vf);
    }

    if (m_pendingFiles.isEmpty()) {
        emit logMessage("✗ 未找到带字幕的视频文件");
        emit finished(false, "未找到带字幕的视频文件");
        return;
    }

    emit logMessage(QString("找到 %1 个带字幕的视频文件，开始合成...").arg(m_pendingFiles.size()));

    if (!m_process) {
        m_process = new QProcess(this);
        m_timer = new QTimer(this);
        connect(m_process, &QProcess::readyReadStandardError,
                this, &FFmpegMergeService::onProcessReadyRead);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &FFmpegMergeService::onProcessFinished);
        connect(m_timer, &QTimer::timeout,
                this, &FFmpegMergeService::onProcessTimeout);
    }

    processNextFile();
}

void FFmpegMergeService::cancel()
{
    m_cancelled = true;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void FFmpegMergeService::processNextFile()
{
    m_currentIndex++;
    if (m_cancelled || m_currentIndex >= m_pendingFiles.size()) {
        QString msg = QString("合成完成: 成功 %1, 失败 %2").arg(m_successCount).arg(m_failCount);
        emit logMessage(msg);
        emit finished(true, msg);
        return;
    }

    const auto &vf = m_pendingFiles[m_currentIndex];
    QFileInfo fi(vf.path);

    emit logMessage(QString("[%1/%2] 合成: %3")
        .arg(m_currentIndex + 1).arg(m_pendingFiles.size()).arg(fi.fileName()));

    // Build ffmpeg command with subtitle burn filter
    QStringList args;
    args << "-y";

    // Input video
    args << "-i" << vf.path;

    // Subtitle filter
    QString escapedSub = QString(vf.subtitlePath).replace("\\", "/").replace(":", "\\:");
    QString vfFilter = QString("subtitles='%1':force_style='FontName=Microsoft YaHei,FontSize=20,"
                               "PrimaryColour=&H00FFFFFF,OutlineColour=&H00000000,"
                               "BorderStyle=1,Outline=1,Shadow=0'").arg(escapedSub);
    args << "-vf" << vfFilter;

    args << "-c:a" << "copy";
    args << vf.outputPath;

    PluginLogger::info("ffmpeg args: " + args.join(" "));

    m_process->start(m_ffmpegPath, args);
    m_timer->start(1000);

    emit progress(double(m_currentIndex) / m_pendingFiles.size());
}

void FFmpegMergeService::onProcessReadyRead()
{
    QByteArray data = m_process->readAllStandardError();
    QString output = QString::fromUtf8(data);

    // Extract time progress for display
    static QRegularExpression timeRe(R"(time=(\d+):(\d+):(\d+)\.(\d+))");
    QRegularExpressionMatch m = timeRe.match(output);
    if (m.hasMatch()) {
        int h = m.captured(1).toInt();
        int min = m.captured(2).toInt();
        int s = m.captured(3).toInt();
        int totalSec = h * 3600 + min * 60 + s;
        Q_UNUSED(totalSec)
    }
}

void FFmpegMergeService::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_timer->stop();

    if (m_cancelled) {
        emit logMessage("  ⏹ 已取消");
        processNextFile();
        return;
    }

    const auto &vf = m_pendingFiles[m_currentIndex];
    QFileInfo fi(vf.path);

    if (status == QProcess::NormalExit && exitCode == 0) {
        emit logMessage("  ✓ 合成完成: " + fi.fileName());
        m_successCount++;
        emit fileProcessed(fi.fileName(), true, vf.outputPath);
    } else {
        QString err = QString::fromUtf8(m_process->readAllStandardError());
        emit logMessage("  ✗ 合成失败: " + fi.fileName() + " - " + err.left(100));
        m_failCount++;
        emit fileProcessed(fi.fileName(), false, "");
    }

    emit progress(double(m_currentIndex + 1) / m_pendingFiles.size());
    processNextFile();
}

void FFmpegMergeService::onProcessTimeout()
{
    // Could add progress estimation here based on file size
}
