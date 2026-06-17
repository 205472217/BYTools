#include "SubtitleAdjustController.h"
#include "Logger.h"
#include "SettingsHelper.h"
#include "Config.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <algorithm>

SubtitleAdjustController::SubtitleAdjustController(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
{
    m_matchModel = new MatchPairModel(this);

    // 加载持久化配置
    QSettings &s = pluginGroupSettings("subtitle-adjust");
    s.sync();
    m_mode = s.value("mode", 0).toInt();
    m_videoFolder = s.value("videoFolder").toString();
    m_subtitleFolder = s.value("subtitleFolder").toString();
    m_recursiveVideo = s.value("recursiveVideo", false).toBool();
    m_recursiveSubtitle = s.value("recursiveSubtitle", false).toBool();
    m_overwriteOriginal = s.value("overwriteOriginal", false).toBool();

    // 加载已完成记录
    loadRecords();
}

// ── mode ──

int SubtitleAdjustController::mode() const { return m_mode; }
void SubtitleAdjustController::setMode(int mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        emit modeChanged();
        reset();
        pluginGroupSettings("subtitle-adjust").setValue("mode", mode);
        pluginGroupSettings("subtitle-adjust").sync();
    }
}

// ── 单文件模式 ──

QString SubtitleAdjustController::videoPath() const { return m_videoPath; }
void SubtitleAdjustController::setVideoPath(const QString &path)
{
    if (m_videoPath != path) {
        m_videoPath = path;
        emit videoPathChanged();
    }
}

QString SubtitleAdjustController::subtitlePath() const { return m_subtitlePath; }
void SubtitleAdjustController::setSubtitlePath(const QString &path)
{
    if (m_subtitlePath != path) {
        m_subtitlePath = path;
        emit subtitlePathChanged();
    }
}

// ── 批量模式 ──

QString SubtitleAdjustController::videoFolder() const { return m_videoFolder; }
void SubtitleAdjustController::setVideoFolder(const QString &path)
{
    if (m_videoFolder != path) {
        m_videoFolder = path;
        emit videoFolderChanged();
        pluginGroupSettings("subtitle-adjust").setValue("videoFolder", path);
        pluginGroupSettings("subtitle-adjust").sync();
    }
}

QString SubtitleAdjustController::subtitleFolder() const { return m_subtitleFolder; }
void SubtitleAdjustController::setSubtitleFolder(const QString &path)
{
    if (m_subtitleFolder != path) {
        m_subtitleFolder = path;
        emit subtitleFolderChanged();
        pluginGroupSettings("subtitle-adjust").setValue("subtitleFolder", path);
        pluginGroupSettings("subtitle-adjust").sync();
    }
}

bool SubtitleAdjustController::recursiveVideo() const { return m_recursiveVideo; }
void SubtitleAdjustController::setRecursiveVideo(bool recursive)
{
    if (m_recursiveVideo != recursive) {
        m_recursiveVideo = recursive;
        emit recursiveVideoChanged();
        pluginGroupSettings("subtitle-adjust").setValue("recursiveVideo", recursive);
        pluginGroupSettings("subtitle-adjust").sync();
    }
}

bool SubtitleAdjustController::recursiveSubtitle() const { return m_recursiveSubtitle; }
void SubtitleAdjustController::setRecursiveSubtitle(bool recursive)
{
    if (m_recursiveSubtitle != recursive) {
        m_recursiveSubtitle = recursive;
        emit recursiveSubtitleChanged();
        pluginGroupSettings("subtitle-adjust").setValue("recursiveSubtitle", recursive);
        pluginGroupSettings("subtitle-adjust").sync();
    }
}

// ── 调整状态 ──

qint64 SubtitleAdjustController::offsetMs() const { return m_offsetMs; }
void SubtitleAdjustController::setOffsetMs(qint64 ms)
{
    if (m_offsetMs != ms) {
        m_offsetMs = ms;
        emit offsetMsChanged();
        if (!m_isDirty) {
            setIsDirty(true);
        }
        // 更新当前字幕文本
        if (m_subtitleEntries.isEmpty())
            setCurrentSubtitleText(QString());
    }
}

QString SubtitleAdjustController::currentSubtitleText() const { return m_currentSubtitleText; }
bool SubtitleAdjustController::isDirty() const { return m_isDirty; }
int SubtitleAdjustController::currentMatchIndex() const { return m_currentMatchIndex; }
QString SubtitleAdjustController::currentVideoPath() const { return m_currentVideoPath; }
QString SubtitleAdjustController::currentSubtitlePath() const { return m_currentSubtitlePath; }

MatchPairModel *SubtitleAdjustController::matchModel() const { return m_matchModel; }

// ── 获取指定位置的字幕文本 ──

QString SubtitleAdjustController::getSubtitleTextAt(qint64 positionMs)
{
    if (m_subtitleEntries.isEmpty())
        return QString();

    // 应用偏移：实际字幕时间 = 播放器位置 - 用户偏移
    // offset > 0 → 字幕延后（推迟），offset < 0 → 字幕提前
    qint64 adjustedPos = positionMs - m_offsetMs;

    for (const auto &entry : m_subtitleEntries) {
        if (adjustedPos >= entry.startTime && adjustedPos < entry.endTime) {
            return entry.text;
        }
    }
    return QString();
}

// ── 开始匹配 ──

void SubtitleAdjustController::startMatch()
{
    if (!m_logger)
        return;

    if (m_mode == 0) {
        // 单文件模式：直接添加当前选中的文件对
        if (m_videoPath.isEmpty() || m_subtitlePath.isEmpty()) {
            QString msg = QStringLiteral("请先选择视频文件和字幕文件");
            m_logger->warn(msg);
            emit logMessage(msg);
            return;
        }

        if (!QFileInfo::exists(m_videoPath)) {
            QString msg = QStringLiteral("视频文件不存在: ") + m_videoPath;
            m_logger->warn(msg);
            emit logMessage(msg);
            return;
        }
        if (!QFileInfo::exists(m_subtitlePath)) {
            QString msg = QStringLiteral("字幕文件不存在: ") + m_subtitlePath;
            m_logger->warn(msg);
            emit logMessage(msg);
            return;
        }

        QList<MatchPairModel::MatchPair> pairs;
        int st = hasRecord(m_subtitlePath) ? 1 : 0;
        pairs.append({m_videoPath, m_subtitlePath, st});
        m_matchModel->setPairs(pairs);

        QString msg = QStringLiteral("✓ 已添加映射: %1 → %2")
            .arg(QFileInfo(m_videoPath).fileName(),
                 QFileInfo(m_subtitlePath).fileName());
        m_logger->info(msg);
        emit logMessage(msg);
    } else {
        // 批量模式：扫描文件夹，按文件名前缀匹配
        if (m_videoFolder.isEmpty() || m_subtitleFolder.isEmpty()) {
            QString msg = QStringLiteral("请先选择视频文件夹和字幕文件夹");
            m_logger->warn(msg);
            emit logMessage(msg);
            return;
        }

        emit logMessage(QStringLiteral("正在扫描视频文件夹..."));
        QStringList videoExts = {"*.mp4", "*.mkv", "*.avi", "*.mov", "*.wmv", "*.flv", "*.webm", "*.m4v", "*.ts"};
        QStringList videoFiles;
        collectFiles(m_videoFolder, m_recursiveVideo, videoExts, videoFiles);

        emit logMessage(QStringLiteral("正在扫描字幕文件夹..."));
        QStringList subExts = {"*.srt", "*.ass", "*.ssa"};
        QStringList subFiles;
        collectFiles(m_subtitleFolder, m_recursiveSubtitle, subExts, subFiles);

        // 按文件名主干（不含扩展名）建立索引
        QMap<QString, QString> subMap;
        for (const auto &sf : subFiles) {
            QFileInfo fi(sf);
            QString stem = fi.completeBaseName().toLower();
            subMap.insert(stem, sf);
        }

        // 匹配视频与字幕
        QList<MatchPairModel::MatchPair> pairs;
        int matched = 0;
        for (const auto &vf : videoFiles) {
            QFileInfo fi(vf);
            QString stem = fi.completeBaseName().toLower();
            if (subMap.contains(stem)) {
                pairs.append({vf, subMap[stem], 0});
                matched++;
            }
        }

        m_matchModel->setPairs(pairs);

        // 查 JSON 记录，已有导出记录的标为「已导出」
        for (int i = 0; i < m_matchModel->rowCount(); ++i) {
            if (hasRecord(m_matchModel->at(i).subtitleFile))
                m_matchModel->setStatus(i, 1);
        }

        QString msg = QStringLiteral("✓ 匹配完成: 共扫描 %1 个视频, %2 个字幕, 成功匹配 %3 对")
            .arg(videoFiles.size()).arg(subFiles.size()).arg(matched);
        m_logger->info(msg);
        emit logMessage(msg);

        if (matched == 0) {
            emit logMessage(QStringLiteral("⚠ 未匹配到任何视频-字幕对，请检查文件名是否一致"));
        }
    }
    emit matchCompleted();
}

// ── 开始调整（选中映射行时调用）──

void SubtitleAdjustController::startAdjust(int index)
{
    m_subtitleEntries.clear();
    m_srtFilePath.clear();

    if (index >= 0 && index < m_matchModel->rowCount()) {
        const auto &pair = m_matchModel->at(index);
        setCurrentMatchIndex(index);
        setCurrentVideoPath(pair.videoFile);
        setCurrentSubtitlePath(pair.subtitleFile);

        // 解析字幕文件
        QString subPath = pair.subtitleFile;
        if (!QFileInfo::exists(subPath)) {
            QString msg = QStringLiteral("✗ 字幕文件不存在: ") + subPath;
            m_logger->error(msg);
            emit logMessage(msg);
            return;
        }

        if (!parseSrtFile(subPath)) {
            QString msg = QStringLiteral("✗ 字幕解析失败: ") + QFileInfo(subPath).fileName();
            m_logger->error(msg);
            emit logMessage(msg);
            return;
        }

        m_srtFilePath = subPath;
        QString msg = QStringLiteral("✓ 已加载字幕: %1 (%2 条)")
            .arg(QFileInfo(subPath).fileName())
            .arg(m_subtitleEntries.size());
        m_logger->info(msg);
        emit logMessage(msg);

        emit videoReady(m_currentVideoPath, m_currentSubtitlePath);
    }

    m_offsetMs = 0;
    emit offsetMsChanged();
    setCurrentSubtitleText(QString());
    setIsDirty(false);
}

// ── 导出字幕 ──

void SubtitleAdjustController::exportSubtitle()
{
    if (m_subtitleEntries.isEmpty() || m_srtFilePath.isEmpty()) {
        QString msg = QStringLiteral("✗ 没有已加载的字幕，无法导出");
        m_logger->warn(msg);
        emit logMessage(msg);
        emit exportFinished(false, msg);
        return;
    }

    // 构造输出路径
    QFileInfo fi(m_srtFilePath);
    QString outputPath;
    if (m_overwriteOriginal) {
        outputPath = m_srtFilePath; // 直接覆盖原文件
    } else {
        outputPath = fi.absolutePath() + "/" + fi.completeBaseName() + "_adjusted.srt";
    }

    if (!writeAdjustedSrt(outputPath)) {
        QString msg = QStringLiteral("✗ 导出失败: ") + outputPath;
        m_logger->error(msg);
        emit logMessage(msg);
        emit exportFinished(false, msg);
        return;
    }

    // 标记为已导出
    if (m_currentMatchIndex >= 0) {
        m_matchModel->setStatus(m_currentMatchIndex, 1);
    }

    // 保存已完成记录
    saveRecord(m_currentVideoPath, m_srtFilePath, m_offsetMs);

    setIsDirty(false);

    QString msg = QStringLiteral("✓ 导出成功: %1").arg(QFileInfo(outputPath).fileName());
    m_logger->info(msg);
    emit logMessage(msg);
    emit exportFinished(true, msg);
}

// ── 偏移操作 ──

void SubtitleAdjustController::shiftForward(qint64 ms)
{
    setOffsetMs(m_offsetMs + ms);
}

void SubtitleAdjustController::shiftBackward(qint64 ms)
{
    setOffsetMs(m_offsetMs - ms);
}

// ── 加载视频（单文件模式兼容）──

void SubtitleAdjustController::loadVideo(const QString &videoPath, const QString &subtitlePath)
{
    m_subtitleEntries.clear();
    m_srtFilePath.clear();

    setCurrentVideoPath(videoPath);
    setCurrentSubtitlePath(subtitlePath);

    if (!QFileInfo::exists(subtitlePath)) {
        QString msg = QStringLiteral("✗ 字幕文件不存在: ") + subtitlePath;
        m_logger->error(msg);
        emit logMessage(msg);
        return;
    }

    if (!parseSrtFile(subtitlePath)) {
        QString msg = QStringLiteral("✗ 字幕解析失败: ") + QFileInfo(subtitlePath).fileName();
        m_logger->error(msg);
        emit logMessage(msg);
        return;
    }

    m_srtFilePath = subtitlePath;

    m_offsetMs = 0;
    emit offsetMsChanged();
    setCurrentSubtitleText(QString());
    setIsDirty(false);
    emit videoReady(videoPath, subtitlePath);
}

// ── 重置 ──

void SubtitleAdjustController::reset()
{
    m_subtitleEntries.clear();
    m_srtFilePath.clear();
    m_offsetMs = 0;
    emit offsetMsChanged();
    setCurrentSubtitleText(QString());
    setIsDirty(false);
    setCurrentMatchIndex(-1);
    setCurrentVideoPath(QString());
    setCurrentSubtitlePath(QString());
    m_matchModel->clear();
}

// ── 私有 setter ──

void SubtitleAdjustController::setCurrentSubtitleText(const QString &text)
{
    if (m_currentSubtitleText != text) {
        m_currentSubtitleText = text;
        emit currentSubtitleTextChanged();
    }
}

void SubtitleAdjustController::setIsDirty(bool dirty)
{
    if (m_isDirty != dirty) {
        m_isDirty = dirty;
        emit isDirtyChanged();
    }
}

void SubtitleAdjustController::setCurrentMatchIndex(int index)
{
    if (m_currentMatchIndex != index) {
        m_currentMatchIndex = index;
        emit currentMatchIndexChanged();
    }
}

void SubtitleAdjustController::setCurrentVideoPath(const QString &path)
{
    if (m_currentVideoPath != path) {
        m_currentVideoPath = path;
        emit currentVideoPathChanged();
    }
}

void SubtitleAdjustController::setCurrentSubtitlePath(const QString &path)
{
    if (m_currentSubtitlePath != path) {
        m_currentSubtitlePath = path;
        emit currentSubtitlePathChanged();
    }
}

// ── 解析 SRT 文件 ──

bool SubtitleAdjustController::parseSrtFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    static QRegularExpression timeRe(
        QStringLiteral("(\\d{2}):(\\d{2}):(\\d{2})[,\\.](\\d{3})\\s*-->\\s*"
                       "(\\d{2}):(\\d{2}):(\\d{2})[,\\.](\\d{3})"));

    m_subtitleEntries.clear();

    QStringList lines;
    while (!in.atEnd())
        lines.append(in.readLine());

    int i = 0;
    while (i < lines.size()) {
        // 跳过空行
        if (lines[i].trimmed().isEmpty()) { i++; continue; }

        // 跳过序号行（纯数字）
        bool isNumber = false;
        int seqNo = lines[i].trimmed().toInt(&isNumber);
        if (isNumber && seqNo > 0) {
            i++;
            continue;
        }

        // 尝试匹配时间行
        QRegularExpressionMatch match = timeRe.match(lines[i]);
        if (!match.hasMatch()) { i++; continue; }

        SubtitleEntry entry;
        entry.startTime = parseSrtTime(match.captured(1) + ":" +
                                       match.captured(2) + ":" +
                                       match.captured(3) + "," +
                                       match.captured(4));
        entry.endTime = parseSrtTime(match.captured(5) + ":" +
                                     match.captured(6) + ":" +
                                     match.captured(7) + "," +
                                     match.captured(8));

        i++;

        // 收集文本
        QString text;
        while (i < lines.size() && !lines[i].trimmed().isEmpty()) {
            if (!text.isEmpty()) text += "\n";
            text += lines[i];
            i++;
        }

        entry.text = text.trimmed();
        if (!entry.text.isEmpty())
            m_subtitleEntries.append(entry);

        i++;
    }

    file.close();
    return !m_subtitleEntries.isEmpty();
}

// ── 写入偏移后的 SRT ──

bool SubtitleAdjustController::writeAdjustedSrt(const QString &outputPath)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    for (int i = 0; i < m_subtitleEntries.size(); ++i) {
        const auto &entry = m_subtitleEntries[i];
        qint64 adjStart = entry.startTime + m_offsetMs;
        qint64 adjEnd = entry.endTime + m_offsetMs;

        // 防止负时间
        if (adjStart < 0) adjStart = 0;
        if (adjEnd < 0) adjEnd = 0;

        out << (i + 1) << "\n";
        out << formatSrtTime(adjStart) << " --> " << formatSrtTime(adjEnd) << "\n";
        out << entry.text << "\n\n";
    }

    file.close();
    return true;
}

// ── 时间工具 ──

qint64 SubtitleAdjustController::parseSrtTime(const QString &timeStr)
{
    // Format: HH:MM:SS,mmm
    static QRegularExpression re(QStringLiteral("(\\d+):(\\d{2}):(\\d{2})[,.]?(\\d*)"));
    QRegularExpressionMatch m = re.match(timeStr);
    if (!m.hasMatch()) return 0;

    qint64 h = m.captured(1).toLongLong();
    qint64 min = m.captured(2).toLongLong();
    qint64 sec = m.captured(3).toLongLong();
    qint64 ms = m.captured(4).toLongLong();
    // 如果毫秒不足3位，补齐
    if (m.captured(4).length() == 1) ms *= 100;
    else if (m.captured(4).length() == 2) ms *= 10;

    return h * 3600000 + min * 60000 + sec * 1000 + ms;
}

QString SubtitleAdjustController::formatSrtTime(qint64 ms)
{
    if (ms < 0) ms = 0;
    qint64 h = ms / 3600000;
    qint64 m = (ms % 3600000) / 60000;
    qint64 s = (ms % 60000) / 1000;
    qint64 millis = ms % 1000;

    return QStringLiteral("%1:%2:%3,%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(millis, 3, 10, QLatin1Char('0'));
}

// ── 递归收集文件（Windows 自然排序，深度优先）──

void SubtitleAdjustController::collectFiles(const QString &dirPath, bool recursive,
                                              const QStringList &extensions, QStringList &results)
{
    QDir dir(dirPath);

    // 当前目录的文件（已排序）
    QFileInfoList files = dir.entryInfoList(extensions, QDir::Files, QDir::NoSort);
    naturalSort(files);
    for (const auto &fi : files)
        results.append(fi.absoluteFilePath());

    if (!recursive)
        return;

    // 子目录（已排序），深度优先递归
    QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
    naturalSort(subdirs);
    for (const auto &sd : subdirs)
        collectFiles(sd.absoluteFilePath(), true, extensions, results);
}

// ── 导出选项 ──

bool SubtitleAdjustController::overwriteOriginal() const
{
    return m_overwriteOriginal;
}

void SubtitleAdjustController::setOverwriteOriginal(bool overwrite)
{
    if (m_overwriteOriginal != overwrite) {
        m_overwriteOriginal = overwrite;
        emit overwriteOriginalChanged();
        pluginGroupSettings("subtitle-adjust").setValue("overwriteOriginal", overwrite);
        pluginGroupSettings("subtitle-adjust").sync();
    }
}

// ── 已完成记录管理 ──

QString SubtitleAdjustController::recordsFilePath() const
{
    // 与 config.ini 同级目录
    QFileInfo fi(pluginConfigFilePath());
    return fi.absolutePath() + "/subtitle_adjust_records.json";
}

void SubtitleAdjustController::loadRecords()
{
    m_records.clear();

    QFile file(recordsFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray())
        return;

    for (const QJsonValue &val : doc.array()) {
        QJsonObject obj = val.toObject();
        CompletedRecord rec;
        rec.videoPath = obj.value("videoPath").toString();
        rec.subtitlePath = obj.value("subtitlePath").toString();
        rec.offsetMs = static_cast<qint64>(obj.value("offsetMs").toDouble());
        rec.timestamp = obj.value("timestamp").toString();
        if (!rec.subtitlePath.isEmpty())
            m_records.insert(rec.subtitlePath, rec);
    }
}

void SubtitleAdjustController::saveRecord(const QString &videoPath,
                                           const QString &subtitlePath,
                                           qint64 offsetMs)
{
    if (videoPath.isEmpty() || subtitlePath.isEmpty())
        return;

    CompletedRecord rec;
    rec.videoPath = videoPath;
    rec.subtitlePath = subtitlePath;
    rec.offsetMs = offsetMs;
    rec.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

    // 更新内存索引（覆盖旧记录）
    m_records.insert(subtitlePath, rec);

    // 写入文件
    QJsonArray arr;
    for (auto it = m_records.constBegin(); it != m_records.constEnd(); ++it) {
        const auto &r = it.value();
        QJsonObject obj;
        obj["videoPath"] = r.videoPath;
        obj["subtitlePath"] = r.subtitlePath;
        obj["offsetMs"] = r.offsetMs;
        obj["timestamp"] = r.timestamp;
        arr.append(obj);
    }

    QFile file(recordsFilePath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    file.close();
}

bool SubtitleAdjustController::hasRecord(const QString &subtitlePath) const
{
    return m_records.contains(subtitlePath);
}
