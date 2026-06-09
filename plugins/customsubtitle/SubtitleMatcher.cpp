#include "SubtitleMatcher.h"
#include "PluginLogger.h"
#include <QRegularExpression>
#include <QDirIterator>
#include <QFile>
#include <QMap>

SubtitleMatcher::SubtitleMatcher(QObject *parent)
    : QObject(parent)
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

bool SubtitleMatcher::isVideoFile(const QString &fileName)
{
    QString lower = fileName.toLower();
    static const QStringList exts = {
        ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".ts", ".rmvb"
    };
    for (const QString &ext : exts) {
        if (lower.endsWith(ext)) return true;
    }
    return false;
}

QList<SubtitleMatcher::MatchResult> SubtitleMatcher::matchSubtitles(
    const QString &subtitleDir,
    const QString &videoDir,
    bool recursive,
    const QStringList &videoExts)
{
    QList<MatchResult> results;
    const QStringList &exts = videoExts.isEmpty() ? m_defaultVideoExts : videoExts;

    // 1. Scan subtitle directory
    QMap<QString, QList<QPair<QString, QString>>> subMap; // key -> [(path, name)]
    QDir sDir(subtitleDir);
    if (!sDir.exists()) {
        emit logMessage("✗ 字幕目录不存在: " + subtitleDir);
        emit finished(false, "字幕目录不存在");
        return results;
    }

    emit logMessage("步骤2：扫描字幕目录...");
    int subCount = 0;
    const QFileInfoList sEntries = sDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fi : sEntries) {
        if (!fi.fileName().toLower().endsWith(".srt")) continue;
        subCount++;
        QString key = extractKey(fi.fileName());
        if (key.isEmpty()) {
            emit logMessage("  [跳过] 未能提取关键码: " + fi.fileName());
            continue;
        }
        subMap[key].append({fi.absoluteFilePath(), fi.fileName()});
        emit logMessage("  [字幕] 关键码 " + key + " → " + fi.fileName());
    }

    if (subCount == 0) {
        emit logMessage("✗ 字幕目录中没有 .srt 文件");
        emit finished(false, "字幕目录中没有 .srt 文件");
        return results;
    }

    // 2. Scan video directory
    QMap<QString, QList<QPair<QString, QString>>> vidMap; // key -> [(path, dir, nameNoExt)]
    struct VidInfo {
        QString path;
        QString dir;
        QString nameNoExt;
    };
    QMap<QString, QList<VidInfo>> vidInfoMap;

    emit logMessage("扫描视频目录...");
    int vidCount = 0;
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) flags = QDirIterator::Subdirectories;

    QDirIterator it(videoDir, QDir::Files, flags);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
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

    if (vidCount == 0) {
        emit logMessage("✗ 视频目录中没有视频文件");
        emit finished(false, "视频目录中没有视频文件");
        return results;
    }

    // 3. Match
    emit logMessage("匹配结果：");
    int matchedCount = 0;
    for (auto it = subMap.constBegin(); it != subMap.constEnd(); ++it) {
        const QString &key = it.key();
        if (!vidInfoMap.contains(key)) {
            emit logMessage("  ✗ [" + key + "] 字幕存在但未找到对应视频");
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

            emit logMessage(QString("  ✓ [%1] %2 → %3 => %4")
                .arg(key, mr.subtitleName, mr.newSubtitleName, mr.videoDir));
        }
    }

    if (matchedCount == 0) {
        emit logMessage("✗ 没有字幕能匹配到视频文件");
        emit finished(false, "没有字幕能匹配到视频");
        return results;
    }

    emit logMessage(QString("共匹配 %1 个字幕文件").arg(matchedCount));
    return results;
}

int SubtitleMatcher::executeRename(QList<MatchResult> &results)
{
    int ok = 0;
    for (int i = 0; i < results.size(); ++i) {
        auto &r = results[i];
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
        if (QFile::rename(r.subtitlePath, dest)) {
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
