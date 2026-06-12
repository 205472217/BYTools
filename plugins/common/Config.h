#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QFileInfoList>
#include <QCollator>
#include <algorithm>

// ── 配置文件路径 ──────────────────────────────────────────
// 返回 <exe_dir>/config.ini，所有插件共用同一个配置文件

inline QString pluginConfigFilePath()
{
    return QCoreApplication::applicationDirPath() + "/config.ini";
}

// ── 扩展名列表 ────────────────────────────────────────────
// 统一的视频/字幕扩展名，之前分散在 SubtitleMatcher、FFmpegMergeService、
// VideoReplaceService、VideoSubtitleController 等多处

inline const QStringList &videoExtensions()
{
    static const QStringList exts = {
        ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".ts", ".rmvb"
    };
    return exts;
}

inline const QStringList &subtitleExtensions()
{
    static const QStringList exts = {".srt", ".ass", ".ssa"};
    return exts;
}

// ── 视频文件检测 ──────────────────────────────────────────

inline bool isVideoFile(const QString &fileName)
{
    const QString lower = fileName.toLower();
    for (const QString &ext : videoExtensions()) {
        if (lower.endsWith(ext))
            return true;
    }
    return false;
}

// ── 自然排序 ──────────────────────────────────────────────
// 替代重复 7 次的 QCollator + std::sort 模式

inline void naturalSort(QFileInfoList &list)
{
    QCollator c;
    c.setNumericMode(true);
    std::sort(list.begin(), list.end(),
              [&](const QFileInfo &a, const QFileInfo &b) {
                  return c.compare(a.fileName(), b.fileName()) < 0;
              });
}
