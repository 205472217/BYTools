#include "SubtitleMatcher.h"
#include "Config.h"
#include "Logger.h"
#include <QRegularExpression>
#include <QFile>
#include <QMap>
#include <QCollator>
#include <algorithm>

SubtitleMatcher::SubtitleMatcher(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
{
}

QString SubtitleMatcher::extractKey(const QString &fileName)
{
    // Match pattern: letters + optional separator + digits
    // e.g. aaa-304, aaa 304, ABC123
    static QRegularExpression re(R"(([a-zA-Z]+)[\s\-]*(\d+))");
    QRegularExpressionMatch m = re.match(fileName);
    if (m.hasMatch()) {
        return m.captured(1).toLower() + "-" + m.captured(2);
    }
    return {};
}

QList<SubtitleMatcher::MatchResult> SubtitleMatcher::matchSubtitles(
    const QString &subtitleDir,
    const QString &videoDir,
    bool recursive,
    const QStringList &videoExts)
{
    QList<MatchResult> results;
    const QStringList &exts = videoExts.isEmpty() ? videoExtensions() : videoExts;

    m_logger->info("========== 步骤2：匹配并移动字幕 ==========");
    m_logger->info(QString("字幕目录: %1").arg(subtitleDir));
    m_logger->info(QString("视频目录: %1 (递归=%2)").arg(videoDir).arg(recursive));

    // 1. Scan subtitle directory
    QMap<QString, QList<QPair<QString, QString>>> subMap; // key -> [(path, name)]
    QDir sDir(subtitleDir);
    if (!sDir.exists()) {
        QString err = "✗ 字幕目录不存在: " + subtitleDir;
        emit logMessage(err);
        m_logger->error(err);
        emit finished(false, "字幕目录不存在");
        return results;
    }

    emit logMessage("步骤2：扫描字幕目录...");
    int subCount = 0;
    QFileInfoList sEntries = sDir.entryInfoList(QDir::Files, QDir::NoSort);
    naturalSort(sEntries);
    for (const QFileInfo &fi : sEntries) {
        if (!fi.fileName().toLower().endsWith(".srt")) continue;
        subCount++;
        QString key = extractKey(fi.fileName());
        if (key.isEmpty()) {
            emit logMessage("  [跳过] 未能提取关键码: " + fi.fileName());
            m_logger->warn(QString("  [跳过] 未能提取关键码: %1").arg(fi.fileName()));
            continue;
        }
        subMap[key].append({fi.absoluteFilePath(), fi.fileName()});
        emit logMessage("  [字幕] 关键码 " + key + " → " + fi.fileName());
    }

    if (subCount == 0) {
        QString err = "✗ 字幕目录中没有 .srt 文件: " + subtitleDir;
        emit logMessage(err);
        m_logger->error(err);
        emit finished(false, "字幕目录中没有 .srt 文件");
        return results;
    }
    if (subMap.isEmpty()) {
        QString err = "✗ 有 .srt 文件，但全部未能提取出关键码（文件名需含字母+数字，如 aaa-304）";
        emit logMessage(err);
        m_logger->warn(err);
        emit finished(false, "有 .srt 文件但均未能提取关键码，请检查文件名是否符合 aaa-304 格式");
        return results;
    }
    m_logger->info(QString("字幕目录扫描完成: 共 %1 个 .srt 文件").arg(subCount));

    // 2. Scan video directory
    struct VidInfo {
        QString path;
        QString dir;
        QString nameNoExt;
    };
    QMap<QString, QList<VidInfo>> vidInfoMap;

    emit logMessage("扫描视频目录...");
    int vidCount = 0;
    std::function<void(const QString &)> collectDir;
    collectDir = [&](const QString &dirPath) {
        QDir dir(dirPath);
        QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::NoSort);
        naturalSort(files);
        for (const QFileInfo &fi : files) {
            QString lower = fi.suffix().toLower();
            if (!exts.contains("." + lower)) continue;
            vidCount++;
            QString key = extractKey(fi.completeBaseName());
            if (key.isEmpty()) continue;
            VidInfo vInfo;
            vInfo.path = fi.absoluteFilePath();
            vInfo.dir = fi.absolutePath();
            vInfo.nameNoExt = fi.completeBaseName();
            vidInfoMap[key].append(vInfo);
            emit logMessage("  [视频] 关键码 " + key + " → " + fi.fileName());
        }
        if (!recursive) return;
        QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
        naturalSort(subdirs);
        for (const QFileInfo &subdir : subdirs) {
            collectDir(subdir.absoluteFilePath());
        }
    };
    collectDir(videoDir);

    if (vidCount == 0) {
        QString err = "✗ 视频目录中没有视频文件: " + videoDir;
        emit logMessage(err);
        m_logger->error(err);
        emit finished(false, "视频目录中没有视频文件");
        return results;
    }
    m_logger->info(QString("视频目录扫描完成: 共 %1 个视频文件").arg(vidCount));

    // 3. Match
    emit logMessage("匹配结果：");
    int matchedCount = 0;
    for (auto it = subMap.constBegin(); it != subMap.constEnd(); ++it) {
        const QString &key = it.key();
        if (!vidInfoMap.contains(key)) {
            emit logMessage("  ✗ [" + key + "] 字幕存在但未找到对应视频");
            m_logger->warn(QString("  [无匹配] 关键码 %1 的字幕未找到对应视频").arg(key));
            continue;
        }

        const auto &subList = it.value();
        const auto &vidList = vidInfoMap[key];

        for (int sIdx = 0; sIdx < subList.size(); ++sIdx) {
            int vIdx = qMin(sIdx, vidList.size() - 1);
            const auto &vid = vidList[vIdx];

            MatchResult mr;
            mr.subtitlePath = subList[sIdx].first;
            mr.subtitleName = subList[sIdx].second;
            mr.videoDir = vid.dir;
            mr.newSubtitleName = vid.nameNoExt + ".srt";
            mr.matched = true;
            results.append(mr);
            matchedCount++;

            QString msg = QString("  ✓ [%1] %2 → %3 => %4")
                .arg(key, mr.subtitleName, mr.newSubtitleName, mr.videoDir);
            emit logMessage(msg);
            m_logger->info(msg);
        }
    }

    if (matchedCount == 0) {
        QString err = "✗ 没有字幕能匹配到视频文件（关键码名称不一致或未命名规范）";
        emit logMessage(err);
        m_logger->error(err);
        emit finished(false, "没有字幕能匹配到视频");
        return results;
    }

    m_logger->info(QString("匹配完成: 共匹配 %1 个字幕文件").arg(matchedCount));
    emit logMessage(QString("共匹配 %1 个字幕文件").arg(matchedCount));
    return results;
}

int SubtitleMatcher::executeRename(QList<MatchResult> &results)
{
    int ok = 0;
    for (int i = 0; i < results.size(); ++i) {
        auto &r = results[i];
        if (r.subtitleName == r.newSubtitleName) {
            emit logMessage("  ✓ 文件名已正确: " + r.subtitleName);
            ok++;
            emit progress(double(i + 1) / results.size());
            continue;
        }
        QFileInfo fi(r.subtitlePath);
        QString newPath = fi.absolutePath() + "/" + r.newSubtitleName;
        if (QFile::rename(r.subtitlePath, newPath)) {
            emit logMessage("  ✓ 已重命名: " + r.subtitleName + " → " + r.newSubtitleName);
            r.subtitlePath = newPath;
            r.subtitleName = r.newSubtitleName;
            ok++;
        } else {
            emit logMessage("  ✗ 重命名失败: " + r.subtitleName);
        }
        emit progress(double(i + 1) / results.size());
    }
    return ok;
}

int SubtitleMatcher::executeMove(const QList<MatchResult> &results)
{
    int ok = 0;
    for (int i = 0; i < results.size(); ++i) {
        const auto &r = results[i];
        QString dest = r.videoDir + "/" + r.newSubtitleName;
        if (r.subtitlePath == dest) {
            emit logMessage("  ✓ 已在目标位置: " + r.newSubtitleName);
            ok++;
        } else if (QFile::rename(r.subtitlePath, dest)) {
            emit logMessage("  ✓ 已移动: " + r.newSubtitleName + " → " + r.videoDir);
            ok++;
        } else {
            // Try copy + delete as fallback
            if (QFile::copy(r.subtitlePath, dest)) {
                QFile::remove(r.subtitlePath);
                emit logMessage("  ✓ 已复制+删除: " + r.newSubtitleName + " → " + r.videoDir);
                ok++;
            } else {
                emit logMessage("  ✗ 移动失败: " + r.newSubtitleName);
            }
        }
        emit progress(double(i + 1) / results.size());
    }
    return ok;
}
