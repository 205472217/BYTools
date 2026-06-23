#include "VideoReplaceService.h"
#include "Config.h"
#include "Logger.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QCollator>
#include <algorithm>
#include <QFile>
#include <QAtomicInt>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUuid>

// ── 辅助函数（定义在使用前） ──

/// 在 rootDir 下递归搜索同名文件（同 Python 的 Path.rglob）
static QString findFileRecursive(const QString &rootDir, const QString &fileName)
{
    QDirIterator it(rootDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (it.fileName() == fileName) {
            return it.filePath();
        }
    }
    return {};
}

// ── VideoReplaceService ──

VideoReplaceService::VideoReplaceService(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
{
    // Worker 线程启动后执行 doWork
    connect(&m_workerThread, &QThread::started, this, &VideoReplaceService::doWork, Qt::DirectConnection);
    connect(&m_workerThread, &QThread::finished, this, [this]() {
        m_workerRunning = false;
    });
}

VideoReplaceService::~VideoReplaceService()
{
    cancel();
    m_workerThread.quit();
    m_workerThread.wait(5000);
}

void VideoReplaceService::startReplace(const QString &videoDir,
                                        const QString &mergedDir,
                                        bool recursive,
                                        bool removeSrt,
                                        bool backupOriginal)
{
    if (m_workerRunning) {
        emit logMessage("✗ 已有替换任务正在执行");
        return;
    }

    m_videoDir = videoDir;
    m_mergedDir = mergedDir;
    m_recursive = recursive;
    m_removeSrt = removeSrt;
    m_backupOriginal = backupOriginal;
    m_cancelled.storeRelaxed(0);
    m_totalVideoCount = 0;
    m_successCount = 0;
    m_failCount = 0;
    m_items.clear();

    m_logger->info(QString("========== 步骤4：替换原视频 =========="));

    // 基本校验：合成目录必须存在
    QDir mDir(mergedDir);
    if (!mDir.exists()) {
        QString err = "✗ 合成视频目录不存在: " + mergedDir;
        emit logMessage(err);
        m_logger->error(err);
        emit finished(false, "合成视频目录不存在");
        return;
    }

    emit logMessage(QString("步骤4：扫描原视频目录..."));
    m_logger->info(QString("原视频目录: %1 (递归=%2)").arg(videoDir).arg(recursive));
    m_logger->info(QString("合成视频目录: %1").arg(mergedDir));

    // 扫描+替换全部在后台线程执行，不阻塞 UI
    m_workerRunning = true;
    m_workerThread.start();
}

void VideoReplaceService::requestStop()
{
    m_cancelled.storeRelaxed(1);
    emit logMessage("⏹ 请求停止：完成当前任务后停止");
}

void VideoReplaceService::cancel()
{
    m_cancelled.storeRelaxed(1);
    if (m_workerRunning) {
        m_workerThread.quit();
        m_workerThread.wait(3000);
        // 等待超时后强制终止
        if (m_workerRunning) {
            m_workerThread.terminate();
            m_workerThread.wait(3000);
        }
    }
}

void VideoReplaceService::doWork()
{
    m_logger->info(QString("开始扫描原视频目录..."));

    // ── 扫描阶段（原在 startReplace 中阻塞 UI，现移入工作线程）──
    int matchedCount = 0;
    int srtCount = 0;

    if (m_recursive)
        emit logMessage("  正在递归查找原视频文件...");
    else
        emit logMessage("  正在扫描原视频文件...");

    std::function<void(const QString &)> collectDir;
    collectDir = [&](const QString &dirPath) {
        QDir dir(dirPath);
        QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::NoSort);
        naturalSort(files);
        for (const QFileInfo &fi : files) {
            if (!isVideoFile(fi.fileName())) continue;

            m_totalVideoCount++;

            // 在 mergedDir 中递归搜索同名文件
            QString mergedPath = findFileRecursive(m_mergedDir, fi.fileName());
            if (mergedPath.isEmpty()) {
                // 不记录详细文件名，减少日志文件大小
                //m_logger->info(QString("  [无合成] %1 (%2 MB) — FFOutput 中未找到同名文件") .arg(fi.absoluteFilePath()) .arg(fi.size() / 1024.0 / 1024.0, 0, 'f', 2));
                continue;
            }

            QFileInfo mergedFi(mergedPath);

            ReplaceItem item;
            item.originalPath = fi.absoluteFilePath();
            item.mergedPath = mergedPath;

            // 检查原文件所在目录是否有关联的 .srt
            QString srtCandidate = fi.absolutePath() + "/" + fi.completeBaseName() + ".srt";
            if (QFileInfo::exists(srtCandidate)) {
                item.srtPath = srtCandidate;
                srtCount++;
            }

            m_items.append(item);
            matchedCount++;

            QString log = QString("  [匹配] %1 | "
                                  "  原视频: %2 (%3 MB) | "
                                  "  合成后: %4 (%5 MB) | ")
                .arg(fi.fileName(),
                     fi.absoluteFilePath(),
                     QString::number(fi.size() / 1024.0 / 1024.0, 'f', 2),
                     mergedPath,
                     QString::number(mergedFi.size() / 1024.0 / 1024.0, 'f', 2));
            if (!item.srtPath.isEmpty()) {
                log += QString("  字幕: %1 ").arg(item.srtPath);
            }
            emit logMessage(log);
            m_logger->info(log);
        }

        if (!m_recursive) return;

        // 子目录按自然名称排序后递归
        QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
        naturalSort(subdirs);
        for (const QFileInfo &subdir : subdirs) {
            if (m_cancelled.loadRelaxed()) return;
            collectDir(subdir.absoluteFilePath());
        }
    };

    collectDir(m_videoDir);

    if (m_items.isEmpty()) {
        QString msg = QString("✗ 未找到可替换的视频文件（原视频 %1 个，但合成目录中无匹配文件）")
            .arg(m_totalVideoCount);
        emit logMessage(msg);
        m_logger->error(msg);
        emit finished(false, "未找到可替换的视频");
        m_workerThread.quit();
        return;
    }

    emit logMessage(QString("找到 %1 个可替换的视频文件，开始替换...")
        .arg(matchedCount));
    m_logger->info(QString("扫描完成: 原视频共 %1 个, 有合成文件 %2 个, 有关联字幕 %3 个")
        .arg(m_totalVideoCount).arg(matchedCount).arg(srtCount));

    emit scanFinished(matchedCount);

    // ── 替换阶段（原 doWork 逻辑） ──
    m_logger->info(QString("========== 开始替换 %1 个文件 ==========").arg(m_items.size()));

    // 在线程中迭代处理（非递归！），同 Python 的 for 循环
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_cancelled.loadRelaxed())
            break;

        const auto &item = m_items[i];
        QFileInfo origFi(item.originalPath);
        QFileInfo mergedFi(item.mergedPath);

        qint64 origSize = origFi.size();
        qint64 mergedSize = mergedFi.size();

        emit currentFileChanged(item.originalPath);
        emit logMessage(QString("[%1/%2] 替换: %3")
            .arg(i + 1).arg(m_items.size()).arg(origFi.fileName()));

        // === 替换逻辑：先备份到系统临时目录，替换成功后再删除 ===
        QString tempBackupPath;
        {
            QString tempDir = QDir::cleanPath(
                QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                + "/BYTools_replace_backup";
            QDir().mkpath(tempDir);
            tempBackupPath = tempDir + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces)
                             + "_" + origFi.fileName();
            if (!QFile::copy(item.originalPath, tempBackupPath)) {
                tempBackupPath.clear();
                emit logMessage("  [警告] 临时备份失败");
                m_logger->warn(QString("  [备份] 临时备份失败: %1").arg(item.originalPath));
            } else {
                m_logger->info(QString("  [备份] 已备份到: %1").arg(tempBackupPath));
            }
        }

        bool replaced = false;
        bool usedCopy = false;

        QFile::remove(item.originalPath);
        if (QFile::rename(item.mergedPath, item.originalPath)) {
            replaced = true;
            usedCopy = false;
        } else if (QFile::copy(item.mergedPath, item.originalPath)) {
            QFile::remove(item.mergedPath);
            replaced = true;
            usedCopy = true;
        }

        // 替换失败且有备份 → 从临时目录还原
        if (!replaced && !tempBackupPath.isEmpty()) {
            if (QFile::copy(tempBackupPath, item.originalPath)) {
                emit logMessage("  [还原] 已从临时备份还原");
                m_logger->info(QString("  [还原] %1 ← %2").arg(item.originalPath, tempBackupPath));
            } else {
                emit logMessage("  [严重] 还原失败！原文件在: " + tempBackupPath);
                m_logger->error(QString("  [还原] 失败: %1 ← %2").arg(item.originalPath, tempBackupPath));
            }
        }

        // 替换成功后删除临时备份
        if (replaced && !tempBackupPath.isEmpty())
            QFile::remove(tempBackupPath);

        if (replaced) {
            if (usedCopy) {
                emit logMessage(QString("  ✓ 已替换 (跨盘拷贝): %1 (%2 MB → %3 MB)")
                    .arg(origFi.fileName())
                    .arg(origSize / 1024.0 / 1024.0, 0, 'f', 2)
                    .arg(mergedSize / 1024.0 / 1024.0, 0, 'f', 2));
                m_logger->info(QString("  [替换/拷贝] %1 | 原=%2 MB 新=%3 MB")
                    .arg(item.originalPath)
                    .arg(origSize / 1024.0 / 1024.0, 0, 'f', 2)
                    .arg(mergedSize / 1024.0 / 1024.0, 0, 'f', 2));
            } else {
                emit logMessage(QString("  ✓ 已替换 (同盘移动): %1 (%2 MB → %3 MB)")
                    .arg(origFi.fileName())
                    .arg(origSize / 1024.0 / 1024.0, 0, 'f', 2)
                    .arg(mergedSize / 1024.0 / 1024.0, 0, 'f', 2));
                m_logger->info(QString("  [替换/移动] %1 | 原=%2 MB 新=%3 MB")
                    .arg(item.originalPath)
                    .arg(origSize / 1024.0 / 1024.0, 0, 'f', 2)
                    .arg(mergedSize / 1024.0 / 1024.0, 0, 'f', 2));
            }

            // 若 merged 文件还残留（copy 分支可能已删，但防意外）
            if (QFileInfo::exists(item.mergedPath)) {
                QFile::remove(item.mergedPath);
                emit logMessage("  [清理] 已删除合成文件: " + mergedFi.fileName());
                m_logger->info(QString("  [清理] 删除合成文件: %1").arg(item.mergedPath));
            }

            // 删除同名 .srt（同 Python 脚本）
            if (m_removeSrt && !item.srtPath.isEmpty()) {
                QFileInfo srtFi(item.srtPath);
                if (QFile::remove(item.srtPath)) {
                    emit logMessage("  [字幕] 已删除: " + srtFi.fileName());
                    m_logger->info(QString("  [字幕] 删除: %1 (%2 KB)")
                        .arg(item.srtPath)
                        .arg(srtFi.size() / 1024.0, 0, 'f', 1));
                } else {
                    emit logMessage("  [字幕] 删除失败: " + srtFi.fileName());
                    m_logger->warn(QString("  [字幕] 删除失败: %1").arg(item.srtPath));
                }
            }

            {
                QMutexLocker lock(&m_mutex);
                m_successCount++;
            }
        } else {
            QString errMsg = QString("  ✗ 替换失败: %1 (%2 MB)")
                .arg(origFi.fileName())
                .arg(origSize / 1024.0 / 1024.0, 0, 'f', 2);
            emit logMessage(errMsg);
            m_logger->error(QString("  [替换失败] %1 | 原=%2 MB 合成=%3 MB")
                .arg(item.originalPath)
                .arg(origSize / 1024.0 / 1024.0, 0, 'f', 2)
                .arg(mergedSize / 1024.0 / 1024.0, 0, 'f', 2));
            {
                QMutexLocker lock(&m_mutex);
                m_failCount++;
            }
        }

        emit progress(double(i + 1) / m_items.size());
    }

    // ── 最终统计（同 Python 2字幕烧录后覆盖原文件.py）──
    int failed;
    int succeeded;
    {
        QMutexLocker lock(&m_mutex);
        succeeded = m_successCount;
        failed = m_failCount;
    }
    int unmatched = m_totalVideoCount - m_items.size();
    QString summary = QString(
        "═══════════════════════════════════════\n"
        "步骤4 替换完成统计:\n"
        "  原视频总数:   %1 个\n"
        "  有合成文件:   %2 个\n"
        "  ── 已匹配 ──\n"
        "  替换成功:     %3 个\n"
        "  替换失败:     %4 个\n"
        "  ── 未匹配 ──\n"
        "  无对应合成:   %5 个\n"
        "═══════════════════════════════════════\n"
        "替换目录: %6\n"
        "合成目录: %7\n"
        "删除字幕: %8\n"
        "═══════════════════════════════════════\n"
        "详情请查看日志文件")
        .arg(m_totalVideoCount)
        .arg(m_items.size())
        .arg(succeeded)
        .arg(failed)
        .arg(unmatched)
        .arg(m_videoDir, m_mergedDir)
        .arg(m_removeSrt ? "是" : "否");

    emit logMessage(summary);
    m_logger->info(summary);

    emit finished(m_cancelled.loadRelaxed() == 0,
                  QString("替换完成: 成功 %1, 失败 %2, 总计 %3")
                      .arg(succeeded).arg(failed).arg(m_items.size()));

    // 不在这里设 m_workerRunning = false，让 QThread::finished 信号在
    // 主线程中通过 lambda 来设置，避免跨线程 data race
    m_workerThread.quit();
}
