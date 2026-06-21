#include "SubtitleMatcher.h"
#include "Config.h"
#include "Logger.h"
#include <QRegularExpression>
#include <QFile>
#include <QMap>
#include <QCollator>
#include <algorithm>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

SubtitleMatcher::SubtitleMatcher(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
{
    connect(&m_workerThread, &QThread::started, this, &SubtitleMatcher::doWork, Qt::DirectConnection);
    connect(&m_workerThread, &QThread::finished, this, [this]() {
        m_workerRunning = false;
    });
}

SubtitleMatcher::~SubtitleMatcher()
{
    cancel();
    m_workerThread.quit();
    m_workerThread.wait(5000);
}

QString SubtitleMatcher::extractKey(const QString &fileName)
{
    static QRegularExpression re(R"(([a-zA-Z]+)[\s\-]*(\d+))");
    QRegularExpressionMatch m = re.match(fileName);
    if (m.hasMatch()) {
        return m.captured(1).toLower() + "-" + m.captured(2);
    }
    return {};
}

void SubtitleMatcher::startMatchAsync(const QString &subtitleDir,
                                       const QString &videoDir,
                                       bool recursive,
                                       const QStringList &videoExts,
                                       const QStringList &preprocessors)
{
    if (m_workerRunning) {
        emit logMessage("✗ 已有匹配任务正在执行");
        return;
    }

    m_subtitleDir = subtitleDir;
    m_videoDir = videoDir;
    m_recursive = recursive;
    m_videoExts = videoExts;
    m_preprocessors = preprocessors;
    m_cancelled.storeRelaxed(0);

    m_logger->info("========== 步骤2：匹配并移动字幕 ==========");

    m_workerRunning = true;
    m_workerThread.start();
}

void SubtitleMatcher::requestStop()
{
    m_cancelled.storeRelaxed(1);
}

void SubtitleMatcher::cancel()
{
    m_cancelled.storeRelaxed(1);
    if (m_workerRunning) {
        m_workerThread.quit();
        m_workerThread.wait(3000);
        if (m_workerRunning) {
            m_workerThread.terminate();
            m_workerThread.wait(3000);
        }
    }
}

QList<SubtitleMatcher::MatchResult> SubtitleMatcher::doMatch()
{
    QList<MatchResult> results;
    const QStringList &exts = m_videoExts.isEmpty() ? videoExtensions() : m_videoExts;

    // 1. Scan subtitle directory
    QMap<QString, QList<QPair<QString, QString>>> subMap;
    QDir sDir(m_subtitleDir);
    if (!sDir.exists()) {
        emit logMessage("✗ 字幕目录不存在: " + m_subtitleDir);
        m_logger->error("字幕目录不存在: " + m_subtitleDir);
        return results;
    }

    emit logMessage("扫描字幕目录...");
    int subCount = 0;
    QFileInfoList sEntries = sDir.entryInfoList(QDir::Files, QDir::NoSort);
    naturalSort(sEntries);
    for (const QFileInfo &fi : sEntries) {
        if (m_cancelled.loadRelaxed()) return results;
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
        emit logMessage("✗ 字幕目录中没有 .srt 文件: " + m_subtitleDir);
        m_logger->error("字幕目录中没有 .srt 文件: " + m_subtitleDir);
        return results;
    }
    if (subMap.isEmpty()) {
        emit logMessage("✗ 有 .srt 文件，但全部未能提取出关键码（文件名需含字母+数字，如 aaa-304）");
        m_logger->warn("有 .srt 文件但均未能提取关键码");
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
        if (m_cancelled.loadRelaxed()) return;
        QDir dir(dirPath);
        QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::NoSort);
        naturalSort(files);
        for (const QFileInfo &fi : files) {
            if (m_cancelled.loadRelaxed()) return;
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
        if (!m_recursive) return;
        QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
        naturalSort(subdirs);
        for (const QFileInfo &subdir : subdirs) {
            if (m_cancelled.loadRelaxed()) return;
            collectDir(subdir.absoluteFilePath());
        }
    };
    collectDir(m_videoDir);

    if (vidCount == 0) {
        emit logMessage("✗ 视频目录中没有视频文件: " + m_videoDir);
        m_logger->error("视频目录中没有视频文件: " + m_videoDir);
        return results;
    }
    m_logger->info(QString("视频目录扫描完成: 共 %1 个视频文件").arg(vidCount));

    // 3. Match
    emit logMessage("匹配结果：");
    int matchedCount = 0;
    for (auto it = subMap.constBegin(); it != subMap.constEnd(); ++it) {
        if (m_cancelled.loadRelaxed()) return results;
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
        emit logMessage("✗ 没有字幕能匹配到视频文件（关键码名称不一致或未命名规范）");
        m_logger->error("没有字幕能匹配到视频文件");
        return results;
    }

    m_logger->info(QString("匹配完成: 共匹配 %1 个字幕文件").arg(matchedCount));
    emit logMessage(QString("共匹配 %1 个字幕文件").arg(matchedCount));
    return results;
}

void SubtitleMatcher::doWork()
{
    // 1. Match
    QList<MatchResult> results = doMatch();

    if (m_cancelled.loadRelaxed()) {
        emit finished(false, "已取消");
        m_workerThread.quit();
        return;
    }

    if (results.isEmpty()) {
        emit finished(false, "未找到匹配结果");
        m_workerThread.quit();
        return;
    }

    emit scanFinished(results.size());

    // 2. Rename
    emit logMessage("--- 重命名 ---");
    int renamed = 0;
    for (int i = 0; i < results.size(); ++i) {
        if (m_cancelled.loadRelaxed()) break;
        auto &r = results[i];
        if (r.subtitleName == r.newSubtitleName) {
            emit logMessage("  ✓ 文件名已正确: " + r.subtitleName);
            renamed++;
            emit progress(0.3 + 0.3 * double(i + 1) / results.size());
            continue;
        }
        QFileInfo fi(r.subtitlePath);
        QString newPath = fi.absolutePath() + "/" + r.newSubtitleName;
        if (QFile::rename(r.subtitlePath, newPath)) {
            emit logMessage("  ✓ 已重命名: " + r.subtitleName + " → " + r.newSubtitleName);
            r.subtitlePath = newPath;
            r.subtitleName = r.newSubtitleName;
            renamed++;
        } else {
            emit logMessage("  ✗ 重命名失败: " + r.subtitleName);
        }
        emit progress(0.3 + 0.3 * double(i + 1) / results.size());
    }

    emit logMessage(QString("重命名完成: %1 个").arg(renamed));
    if (renamed == 0) {
        emit finished(false, "重命名失败");
        m_workerThread.quit();
        return;
    }

    // 3. Move
    emit logMessage("--- 移动 ---");
    int moved = 0;
    for (int i = 0; i < results.size(); ++i) {
        if (m_cancelled.loadRelaxed()) break;
        const auto &r = results[i];
        QString dest = r.videoDir + "/" + r.newSubtitleName;
        if (r.subtitlePath == dest) {
            emit logMessage("  ✓ 已在目标位置: " + r.newSubtitleName);
            moved++;
        } else if (QFile::rename(r.subtitlePath, dest)) {
            emit logMessage("  ✓ 已移动: " + r.newSubtitleName + " → " + r.videoDir);
            moved++;
        } else {
            if (QFile::copy(r.subtitlePath, dest)) {
                QFile::remove(r.subtitlePath);
                emit logMessage("  ✓ 已复制+删除: " + r.newSubtitleName + " → " + r.videoDir);
                moved++;
            } else {
                emit logMessage("  ✗ 移动失败: " + r.newSubtitleName);
            }
        }
        emit progress(0.6 + 0.3 * double(i + 1) / results.size());
    }

    emit logMessage(QString("移动完成: %1 个").arg(moved));

    // 4. SRT preprocessing
    if (!m_preprocessors.isEmpty()) {
        emit logMessage("--- 字幕内容预处理 ---");
        int processed = 0;
        for (int i = 0; i < results.size(); ++i) {
            if (m_cancelled.loadRelaxed()) break;
            const auto &r = results[i];
            QString destPath = r.videoDir + "/" + r.newSubtitleName;
            processSrtFile(destPath, m_preprocessors);
            ++processed;
            emit progress(0.9 + 0.1 * double(i + 1) / results.size());
        }
        emit logMessage(QString("预处理完成: %1 个文件").arg(processed));
    }

    emit progress(1.0);
    emit finished(true, "");
    m_workerThread.quit();
}

// ── SRT Preprocessing (moved from CustomSubtitleController) ──

QList<SubtitleMatcher::SrtEntry> SubtitleMatcher::parseSrtFile(const QString &filePath)
{
    QList<SrtEntry> entries;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return entries;

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    static QRegularExpression timeRe(R"(^(\d{2}:\d{2}:\d{2}[,.]\d{3})\s*-->\s*(\d{2}:\d{2}:\d{2}[,.]\d{3}))");
    auto parseMs = [](const QString &t) -> qint64 {
        QRegularExpression re(R"((\d{2}):(\d{2}):(\d{2})[,.](\d{3}))");
        auto m = re.match(t);
        if (!m.hasMatch()) return 0;
        return m.captured(1).toLongLong() * 3600000
             + m.captured(2).toLongLong() * 60000
             + m.captured(3).toLongLong() * 1000
             + m.captured(4).toLongLong();
    };

    bool readingTime = false;
    SrtEntry entry;
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty()) {
            if (readingTime && !entry.textLines.isEmpty()) {
                entries.append(entry);
                entry = SrtEntry();
            }
            readingTime = false;
            continue;
        }
        auto tm = timeRe.match(line);
        if (tm.hasMatch()) {
            readingTime = true;
            entry.startMs = parseMs(tm.captured(1));
            entry.endMs = parseMs(tm.captured(2));
        } else if (readingTime) {
            entry.textLines.append(line);
        }
    }
    if (readingTime && !entry.textLines.isEmpty())
        entries.append(entry);

    file.close();
    return entries;
}

bool SubtitleMatcher::writeSrtFile(const QString &filePath, const QList<SrtEntry> &entries)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    auto fmtMs = [](qint64 ms) -> QString {
        qint64 h = ms / 3600000; ms %= 3600000;
        qint64 m = ms / 60000;   ms %= 60000;
        qint64 s = ms / 1000;    ms %= 1000;
        return QString("%1:%2:%3,%4")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0'));
    };

    for (int i = 0; i < entries.size(); ++i) {
        const auto &e = entries[i];
        stream << (i + 1) << "\n";
        stream << fmtMs(e.startMs) << " --> " << fmtMs(e.endMs) << "\n";
        for (int j = 0; j < e.textLines.size(); ++j) {
            if (j > 0) stream << "\n";
            stream << e.textLines[j];
        }
        stream << "\n\n";
    }

    file.close();
    return true;
}

void SubtitleMatcher::processSrtFile(const QString &filePath, const QStringList &ops)
{
    QFileInfo fi(filePath);
    QString fileName = fi.fileName();

    QList<SrtEntry> entries = parseSrtFile(filePath);
    if (entries.isEmpty())
        return;

    int beforeCount = entries.size();

    // ── 去重 ──
    if (ops.contains("dedup")) {
        QList<SrtEntry> deduped;
        QString lastText;
        for (const auto &e : entries) {
            QString cur = e.textLines.join("\n").trimmed();
            if (cur == lastText)
                continue;
            deduped.append(e);
            lastText = cur;
        }
        int removed = entries.size() - deduped.size();
        if (removed > 0)
            emit logMessage(QString("  ✓ [%1] 去重: 移除 %2 条重复字幕").arg(fileName).arg(removed));
        entries = deduped;
    }

    // ── 过滤环境音 ──
    if (ops.contains("removeEnvSound")) {
        const QChar fullLeft(0xFF08);
        const QChar fullRight(0xFF09);
        QList<SrtEntry> filtered;
        for (const auto &e : entries) {
            QString text = e.textLines.join("\n").trimmed();
            bool isEnv = (!text.isEmpty()) &&
                ((text.startsWith('(') && text.endsWith(')')) ||
                 (text.startsWith('[') && text.endsWith(']')) ||
                 (text.startsWith(fullLeft) && text.endsWith(fullRight)));
            if (!isEnv)
                filtered.append(e);
        }
        int removed = entries.size() - filtered.size();
        if (removed > 0)
            emit logMessage(QString("  ✓ [%1] 去除环境音: 移除 %2 条").arg(fileName).arg(removed));
        entries = filtered;
    }

    // ── 过滤背景音/音效 ──
    if (ops.contains("removeBgSound")) {
        QList<SrtEntry> filtered;
        for (const auto &e : entries) {
            QString text = e.textLines.join("\n").trimmed();
            if (!text.isEmpty() && text.startsWith('*') && text.endsWith('*'))
                continue;
            filtered.append(e);
        }
        int removed = entries.size() - filtered.size();
        if (removed > 0)
            emit logMessage(QString("  ✓ [%1] 去除背景音: 移除 %2 条音效").arg(fileName).arg(removed));
        entries = filtered;
    }

    // ── 过滤歌词 ──
    if (ops.contains("removeMusic")) {
        const QChar musicNote(0x266A);
        QList<SrtEntry> filtered;
        for (const auto &e : entries) {
            QString text = e.textLines.join("\n").trimmed();
            if (!text.isEmpty() && text.startsWith(musicNote) && text.endsWith(musicNote))
                continue;
            filtered.append(e);
        }
        int removed = entries.size() - filtered.size();
        if (removed > 0)
            emit logMessage(QString("  ✓ [%1] 过滤歌词: 移除 %2 条歌词").arg(fileName).arg(removed));
        entries = filtered;
    }

    // ── 中文繁转简 ──
    if (ops.contains("t2s")) {
#ifdef Q_OS_WIN
        int convertedCount = 0;
        for (auto &e : entries) {
            for (auto &line : e.textLines) {
                QString orig = line;
                int len = line.length();
                int req = LCMapStringEx(
                    L"zh-CN", LCMAP_SIMPLIFIED_CHINESE,
                    reinterpret_cast<LPCWSTR>(line.utf16()), len,
                    nullptr, 0, nullptr, nullptr, 0);
                if (req <= 0) continue;
                QString out(req, Qt::Uninitialized);
                int ret = LCMapStringEx(
                    L"zh-CN", LCMAP_SIMPLIFIED_CHINESE,
                    reinterpret_cast<LPCWSTR>(line.utf16()), len,
                    reinterpret_cast<LPWSTR>(out.data()), req,
                    nullptr, nullptr, 0);
                if (ret > 0) {
                    out.truncate(ret);
                    if (out != orig) {
                        line = out;
                        ++convertedCount;
                    }
                }
            }
        }
        if (convertedCount > 0)
            emit logMessage(QString("  ✓ [%1] 繁转简: 转换 %2 行").arg(fileName).arg(convertedCount));
#else
        emit logMessage(QString("  - [%1] 繁转简: 当前平台不支持，跳过").arg(fileName));
#endif
    }

    // ── 有变更则写回 ──
    if (entries.size() != beforeCount) {
        writeSrtFile(filePath, entries);
        emit logMessage(QString("  ✓ [%1] 处理完成: %2 → %3 条").arg(fileName).arg(beforeCount).arg(entries.size()));
    } else if (ops.contains("t2s")) {
        writeSrtFile(filePath, entries);
    } else {
        emit logMessage(QString("  - [%1] 无需处理").arg(fileName));
    }
}
