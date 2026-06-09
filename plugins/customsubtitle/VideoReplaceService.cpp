#include "VideoReplaceService.h"
#include "PluginLogger.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>

VideoReplaceService::VideoReplaceService(QObject *parent)
    : QObject(parent)
{
}

void VideoReplaceService::startReplace(const QString &videoDir,
                                        const QString &mergedDir,
                                        bool recursive,
                                        bool removeSrt,
                                        bool backupOriginal)
{
    m_videoDir = videoDir;
    m_mergedDir = mergedDir;
    m_recursive = recursive;
    m_removeSrt = removeSrt;
    m_backupOriginal = backupOriginal;
    m_cancelled = false;
    m_currentIndex = -1;
    m_successCount = 0;
    m_failCount = 0;
    m_items.clear();

    QDir mDir(mergedDir);
    if (!mDir.exists()) {
        emit logMessage("✗ 合成视频目录不存在: " + mergedDir);
        emit finished(false, "合成视频目录不存在");
        return;
    }

    // Scan original video directory
    emit logMessage("步骤4：扫描原视频目录...");
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) flags = QDirIterator::Subdirectories;

    QDirIterator it(videoDir, QDir::Files, flags);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
        QString lower = fi.suffix().toLower();
        if (!m_videoExts.contains("." + lower)) continue;

        // Look for matching merged file in mergedDir (by filename)
        QString mergedPath = mergedDir + "/" + fi.fileName();
        if (!QFileInfo::exists(mergedPath)) continue;

        ReplaceItem item;
        item.originalPath = fi.absoluteFilePath();
        item.mergedPath = mergedPath;

        // Look for matching .srt next to original
        QString srtCandidate = fi.absolutePath() + "/" + fi.completeBaseName() + ".srt";
        if (QFileInfo::exists(srtCandidate)) {
            item.srtPath = srtCandidate;
        }

        m_items.append(item);
        emit logMessage(QString("  [匹配] %1 ← %2").arg(fi.fileName(), mergedPath));
    }

    if (m_items.isEmpty()) {
        emit logMessage("✗ 未找到可替换的视频文件（合成目录中无匹配文件）");
        emit finished(false, "未找到可替换的视频");
        return;
    }

    emit logMessage(QString("找到 %1 个可替换的视频文件，开始替换...").arg(m_items.size()));
    processNextFile();
}

void VideoReplaceService::cancel()
{
    m_cancelled = true;
}

void VideoReplaceService::processNextFile()
{
    if (m_cancelled || m_currentIndex >= m_items.size()) {
        QString msg = QString("替换完成: 成功 %1, 失败 %2").arg(m_successCount).arg(m_failCount);
        emit logMessage(msg);
        emit finished(true, msg);
        return;
    }

    m_currentIndex++;
    if (m_currentIndex >= m_items.size()) {
        processNextFile();
        return;
    }

    const auto &item = m_items[m_currentIndex];
    QFileInfo origFi(item.originalPath);
    QFileInfo mergedFi(item.mergedPath);

    emit logMessage(QString("[%1/%2] 替换: %3")
        .arg(m_currentIndex + 1).arg(m_items.size()).arg(origFi.fileName()));

    // Backup original if requested
    if (m_backupOriginal) {
        QString backupPath = origFi.absolutePath() + "/" + origFi.completeBaseName() + "_backup." + origFi.suffix();
        if (QFile::copy(item.originalPath, backupPath)) {
            emit logMessage("  [备份] 已备份原文件: " + backupPath);
        } else {
            emit logMessage("  [警告] 备份失败（跳过）");
        }
    }

    // Replace original with merged file
    // Remove original first, then copy merged to original location
    QFile::remove(item.originalPath);
    if (QFile::copy(item.mergedPath, item.originalPath)) {
        emit logMessage("  ✓ 已替换: " + origFi.fileName());
        m_successCount++;

        // Remove merged file
        QFile::remove(item.mergedPath);
        emit logMessage("  [清理] 已删除合成文件: " + mergedFi.fileName());

        // Remove .srt if exists and requested
        if (m_removeSrt && !item.srtPath.isEmpty()) {
            QFile::remove(item.srtPath);
            emit logMessage("  [字幕] 已删除同名字幕: " + QFileInfo(item.srtPath).fileName());
        }
    } else {
        emit logMessage("  ✗ 替换失败: " + origFi.fileName());
        m_failCount++;
    }

    emit progress(double(m_currentIndex + 1) / m_items.size());
    processNextFile();
}
