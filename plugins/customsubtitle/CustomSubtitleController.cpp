#include "CustomSubtitleController.h"
#include "CustomSubtitlePlugin.h"
#include "SubtitleMatcher.h"
#include "FFmpegMergeService.h"
#include "FfmpegUtils.h"
#include "VideoReplaceService.h"
#include "Config.h"
#include "Logger.h"
#include "SettingsHelper.h"
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>
#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

CustomSubtitleController::CustomSubtitleController(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_matcher(new SubtitleMatcher(m_logger, this))
    , m_mergeService(new FFmpegMergeService(m_logger, this))
    , m_replaceService(new VideoReplaceService(m_logger, this))
{
    // Connect matcher signals
    connect(m_matcher, &SubtitleMatcher::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_matcher, &SubtitleMatcher::progress,
            this, &CustomSubtitleController::setProgress);
    connect(m_matcher, &SubtitleMatcher::progress,
            this, [this](double value) {
        if (m_totalCount > 0)
            setProcessedCount(qMin(m_totalCount, qRound(value * m_totalCount)));
    });
    connect(m_matcher, &SubtitleMatcher::finished,
            this, &CustomSubtitleController::onMatchFinished);

    // Connect merge service signals
    connect(m_mergeService, &FFmpegMergeService::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_mergeService, &FFmpegMergeService::progress,
            this, &CustomSubtitleController::setProgress);
    connect(m_mergeService, &FFmpegMergeService::finished,
            this, &CustomSubtitleController::onMergeFinished);
    connect(m_mergeService, &FFmpegMergeService::currentFileChanged,
            this, [this](const QString &path) {
        setCurrentFile(path);
    });
    connect(m_mergeService, &FFmpegMergeService::currentFileProgress,
            this, &CustomSubtitleController::setCurrentFileProgress);
    // Step 3 显示进度条+百分比，不追踪 N/M 计数

    // Connect replace service signals
    connect(m_replaceService, &VideoReplaceService::logMessage,
            this, &CustomSubtitleController::logMessage);
    connect(m_replaceService, &VideoReplaceService::progress,
            this, &CustomSubtitleController::setProgress);
    connect(m_replaceService, &VideoReplaceService::finished,
            this, &CustomSubtitleController::onReplaceFinished);
    connect(m_replaceService, &VideoReplaceService::scanFinished,
            this, [this](int matched) {
        setTotalCount(matched);
    });
    connect(m_replaceService, &VideoReplaceService::progress,
            this, [this](double value) {
        if (m_totalCount > 0)
            setProcessedCount(qMin(m_totalCount, qRound(value * m_totalCount)));
    });
    connect(m_replaceService, &VideoReplaceService::currentFileChanged,
            this, [this](const QString &path) {
        setCurrentFile(path);
    });

    // Load persisted paths
    QSettings &s = pluginGroupSettings(CustomSubtitlePlugin::kIniSection);
    s.sync();
    m_subtitleDownloadPath = s.value("customSubtitleDownloadPath").toString();
    m_videoSourcePath = s.value("customVideoSourcePath").toString();
    m_mergedOutputPath = s.value("customMergedOutputPath").toString();
    m_ffmpegPath = s.value("customFfmpegPath").toString();
    m_recursive = s.value("customRecursive", false).toBool();
    m_gpuAccel = s.value("customGpuAccel", false).toBool();
    m_enabledPreprocessors = s.value("customPreprocessors").toStringList();
}

CustomSubtitleController::~CustomSubtitleController() = default;

// ── Getters ──
QString CustomSubtitleController::subtitleDownloadPath() const { return m_subtitleDownloadPath; }
QString CustomSubtitleController::videoSourcePath() const { return m_videoSourcePath; }
bool CustomSubtitleController::recursive() const { return m_recursive; }
QString CustomSubtitleController::mergedOutputPath() const { return m_mergedOutputPath; }
bool CustomSubtitleController::gpuAccel() const { return m_gpuAccel; }
bool CustomSubtitleController::removeSrtAfterReplace() const { return m_removeSrtAfterReplace; }
bool CustomSubtitleController::backupOriginal() const { return m_backupOriginal; }
QString CustomSubtitleController::statusMessage() const { return m_statusMessage; }
double CustomSubtitleController::progress() const { return m_progress; }
double CustomSubtitleController::currentFileProgress() const { return m_currentFileProgress; }
bool CustomSubtitleController::isProcessing() const { return m_isProcessing; }
QString CustomSubtitleController::currentStep() const { return m_currentStep; }
int CustomSubtitleController::processedCount() const { return m_processedCount; }
int CustomSubtitleController::totalCount() const { return m_totalCount; }
QString CustomSubtitleController::currentFile() const { return m_currentFile; }

// ── Setters ──
void CustomSubtitleController::setSubtitleDownloadPath(const QString &path)
{
    if (m_subtitleDownloadPath != path) {
        m_subtitleDownloadPath = path;
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).setValue("customSubtitleDownloadPath", path);
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).sync();
        emit subtitleDownloadPathChanged();
    }
}

void CustomSubtitleController::setVideoSourcePath(const QString &path)
{
    if (m_videoSourcePath != path) {
        m_videoSourcePath = path;
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).setValue("customVideoSourcePath", path);
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).sync();
        emit videoSourcePathChanged();
    }
}

void CustomSubtitleController::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).setValue("customRecursive", recursive);
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).sync();
        emit recursiveChanged();
    }
}

void CustomSubtitleController::setMergedOutputPath(const QString &path)
{
    if (m_mergedOutputPath != path) {
        m_mergedOutputPath = path;
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).setValue("customMergedOutputPath", path);
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).sync();
        emit mergedOutputPathChanged();
    }
}

void CustomSubtitleController::setFfmpegPath(const QString &path)
{
    if (m_ffmpegPath != path) {
        m_ffmpegPath = path;
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).setValue("customFfmpegPath", path);
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).sync();
        emit ffmpegPathChanged();
    }
}

void CustomSubtitleController::setGpuAccel(bool enable)
{
    if (m_gpuAccel != enable) {
        m_gpuAccel = enable;
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).setValue("customGpuAccel", enable);
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).sync();
        emit gpuAccelChanged();
    }
}

void CustomSubtitleController::setRemoveSrtAfterReplace(bool remove)
{
    if (m_removeSrtAfterReplace != remove) {
        m_removeSrtAfterReplace = remove;
        emit removeSrtAfterReplaceChanged();
    }
}

void CustomSubtitleController::setBackupOriginal(bool backup)
{
    if (m_backupOriginal != backup) {
        m_backupOriginal = backup;
        emit backupOriginalChanged();
    }
}

// ── Preprocessing ──

QStringList CustomSubtitleController::enabledPreprocessors() const
{
    return m_enabledPreprocessors;
}

void CustomSubtitleController::setEnabledPreprocessors(const QStringList &ops)
{
    if (m_enabledPreprocessors != ops) {
        m_enabledPreprocessors = ops;
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).setValue("customPreprocessors", ops);
        pluginGroupSettings(CustomSubtitlePlugin::kIniSection).sync();
        emit enabledPreprocessorsChanged();
    }
}

// ── Actions ──

void CustomSubtitleController::matchAndMoveSubtitles()
{
    if (m_isProcessing) return;

    if (m_subtitleDownloadPath.isEmpty() || m_videoSourcePath.isEmpty()) {
        setStatusMessage("请先配置字幕下载路径和原视频路径");
        return;
    }

    setCurrentStep("匹配并移动字幕");
    setIsProcessing(true);
    setProgress(0.0);
    emit logMessage("========== 步骤2：匹配并移动字幕 ==========");

    m_logger->info(QString("匹配字幕: %1 → %2 (递归=%3)")
        .arg(m_subtitleDownloadPath, m_videoSourcePath).arg(m_recursive));

    // Run matching (blocking in current thread for simplicity; for large dirs use QThread)
    // 注意：matchSubtitles 内部出错会 emit finished() → 同步触发 onMatchFinished → 设 isProcessing=false
    QList<SubtitleMatcher::MatchResult> results = m_matcher->matchSubtitles(
        m_subtitleDownloadPath, m_videoSourcePath, m_recursive, {});

    // matchSubtitles 的 finished 信号已经处理了错误，不再重复处理
    if (!m_isProcessing)
        return;

    if (results.isEmpty()) {
        onMatchFinished(false, "未找到匹配结果");
        return;
    }

    setTotalCount(results.size());
    setProcessedCount(0);

    // Execute rename
    emit logMessage("--- 重命名 ---");
    int renamed = m_matcher->executeRename(results);
    emit logMessage(QString("重命名完成: %1 个").arg(renamed));

    if (renamed == 0) {
        setIsProcessing(false);
        setCurrentStep("");
        onMatchFinished(false, "重命名失败");
        return;
    }

    // Execute move
    emit logMessage("--- 移动 ---");
    int moved = m_matcher->executeMove(results);
    emit logMessage(QString("移动完成: %1 个").arg(moved));

    // Apply subtitle content preprocessing
    if (!m_enabledPreprocessors.isEmpty()) {
        emit logMessage("--- 字幕内容预处理 ---");
        int processed = 0;
        for (const auto &r : results) {
            QString destPath = r.videoDir + "/" + r.newSubtitleName;
            processSrtFile(destPath, m_enabledPreprocessors);
            ++processed;
        }
        emit logMessage(QString("预处理完成: %1 个文件").arg(processed));
    }

    setProgress(1.0);
    setStatusMessage(QString("步骤2完成: 重命名 %1, 移动 %2").arg(renamed).arg(moved));
    setIsProcessing(false);
    setCurrentStep("");
    onMatchFinished(true, "");
}

void CustomSubtitleController::mergeSubtitleToVideo()
{
    if (m_isProcessing) return;

    QString ffmpeg = ffmpegPath();
    if (ffmpeg.isEmpty() || !isFFmpegAvailable(ffmpeg, m_logger)) {
        setStatusMessage("FFmpeg 不可用，请在自定义字幕设置中配置 FFmpeg 路径");
        emit logMessage("✗ FFmpeg 不可用");
        return;
    }

    if (m_videoSourcePath.isEmpty() || m_mergedOutputPath.isEmpty()) {
        setStatusMessage("请先配置原视频路径和合成视频路径");
        return;
    }

    // Ensure output dir exists
    QDir().mkpath(m_mergedOutputPath);

    setCurrentStep("合成视频+字幕");
    setIsProcessing(true);
    setProgress(0.0);
    emit logMessage("========== 步骤3：合成视频+字幕 ==========");
    m_logger->info(QString("合成: %1 → %2 (GPU=%3)")
        .arg(m_videoSourcePath, m_mergedOutputPath).arg(m_gpuAccel));

    m_mergeService->startMerge(ffmpeg, m_videoSourcePath, m_mergedOutputPath,
                                m_recursive, m_gpuAccel);
}

void CustomSubtitleController::replaceOriginalVideo()
{
    if (m_isProcessing) return;

    if (m_mergedOutputPath.isEmpty() || m_videoSourcePath.isEmpty()) {
        setStatusMessage("请先配置合成视频路径和原视频路径");
        return;
    }

    setCurrentStep("替换原视频");
    setIsProcessing(true);
    setProgress(0.0);
    setProcessedCount(0);
    setTotalCount(0);
    emit logMessage("========== 步骤4：替换原视频 ==========");
    emit logMessage(QString("原视频目录: %1 (递归=%2)").arg(m_videoSourcePath).arg(m_recursive));
    emit logMessage(QString("合成视频目录: %1").arg(m_mergedOutputPath));
    m_logger->info(QString("========== 步骤4：替换原视频 =========="));
    m_logger->info(QString("原视频目录: %1 (递归=%2)").arg(m_videoSourcePath).arg(m_recursive));
    m_logger->info(QString("合成视频目录: %1").arg(m_mergedOutputPath));
    m_logger->info(QString("删除字幕=%1 | 备份原文件=%2")
        .arg(m_removeSrtAfterReplace).arg(m_backupOriginal));

    m_replaceService->startReplace(m_videoSourcePath, m_mergedOutputPath,
                                    m_recursive, m_removeSrtAfterReplace, m_backupOriginal);
}

void CustomSubtitleController::cancel()
{
    m_mergeService->cancel();
    m_replaceService->cancel();

    setIsProcessing(false);
    setStatusMessage("已取消");
    setCurrentStep("");
    setProcessedCount(0);
    setTotalCount(0);
    emit logMessage("⏹ 已取消操作");
}

void CustomSubtitleController::requestStopAfterCount(int count)
{
    if (m_mergeService)
        m_mergeService->requestStopAfterCount(count);
}

void CustomSubtitleController::reset()
{
    cancel();
    setStatusMessage("");
    setCurrentStep("");
    setProgress(0.0);
    setCurrentFileProgress(0.0);
}

QString CustomSubtitleController::ffmpegPath() const
{
    return m_ffmpegPath;
}

// ── Slots ──

void CustomSubtitleController::onMatchFinished(bool success, const QString &error)
{
    if (!success) {
        setStatusMessage("步骤2失败: " + error);
        emit logMessage("✗ 步骤2失败: " + error);
        m_logger->error(QString("步骤2失败: %1").arg(error));
    } else {
        m_logger->info("步骤2完成 ✓");
    }
    setIsProcessing(false);
    setCurrentStep("");
}

void CustomSubtitleController::onMergeFinished(bool success, const QString &error)
{
    if (success) {
        setStatusMessage("步骤3完成");
        emit logMessage("✓ 步骤3：合成完成");
        m_logger->info("步骤3完成 ✓");
    } else {
        setStatusMessage(error);
        emit logMessage("✗ 步骤3失败: " + error);
        m_logger->error(QString("步骤3失败: %1").arg(error));
    }
    setIsProcessing(false);
    setCurrentStep("");
}

void CustomSubtitleController::onReplaceFinished(bool success, const QString &error)
{
    if (success) {
        setStatusMessage("步骤4完成");
        emit logMessage("✓ 步骤4：替换完成");
        m_logger->info("步骤4完成 ✓");
    } else {
        setStatusMessage(error);
        emit logMessage("✗ 步骤4失败: " + error);
        m_logger->error(QString("步骤4失败: %1").arg(error));
    }
    setIsProcessing(false);
    setCurrentStep("");
}

// ── Internal ──

void CustomSubtitleController::setStatusMessage(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged();
    }
}

void CustomSubtitleController::setCurrentStep(const QString &step)
{
    if (m_currentStep != step) {
        m_currentStep = step;
        emit currentStepChanged();
    }
}

void CustomSubtitleController::setProgress(double value)
{
    value = qBound(0.0, value, 1.0);
    if (!qFuzzyCompare(m_progress, value)) {
        m_progress = value;
        emit progressChanged();
    }
}

void CustomSubtitleController::setCurrentFileProgress(double value)
{
    value = qBound(0.0, value, 1.0);
    if (!qFuzzyCompare(m_currentFileProgress, value)) {
        m_currentFileProgress = value;
        emit currentFileProgressChanged();
    }
}

void CustomSubtitleController::setProcessedCount(int count)
{
    if (m_processedCount != count) {
        m_processedCount = count;
        emit processedCountChanged();
    }
}

void CustomSubtitleController::setTotalCount(int count)
{
    if (m_totalCount != count) {
        m_totalCount = count;
        emit totalCountChanged();
    }
}

void CustomSubtitleController::setCurrentFile(const QString &path)
{
    if (m_currentFile != path) {
        m_currentFile = path;
        emit currentFileChanged();
    }
}

// ── SRT 预处理 ──

QList<CustomSubtitleController::SrtEntry> CustomSubtitleController::parseSrtFile(const QString &filePath)
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

bool CustomSubtitleController::writeSrtFile(const QString &filePath, const QList<SrtEntry> &entries)
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

void CustomSubtitleController::processSrtFile(const QString &filePath, const QStringList &ops)
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
        const QChar fullLeft(0xFF08);   // （
        const QChar fullRight(0xFF09);  // ）
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
        const QChar musicNote(0x266A); // ♪
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
        // t2s 可能改变了文本但条数不变，仍需要写回
        writeSrtFile(filePath, entries);
    } else {
        emit logMessage(QString("  - [%1] 无需处理").arg(fileName));
    }
}

void CustomSubtitleController::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        if (!processing)
            setCurrentFile("");
        emit isProcessingChanged();
    }
}
