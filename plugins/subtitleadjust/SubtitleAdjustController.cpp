#include "SubtitleAdjustController.h"
#include "Logger.h"

SubtitleAdjustController::SubtitleAdjustController(PluginLogger *logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
{
    m_matchModel = new MatchPairModel(this);
}

int SubtitleAdjustController::mode() const { return m_mode; }
void SubtitleAdjustController::setMode(int mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        emit modeChanged();
        reset();
    }
}

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

QString SubtitleAdjustController::videoFolder() const { return m_videoFolder; }
void SubtitleAdjustController::setVideoFolder(const QString &path)
{
    if (m_videoFolder != path) {
        m_videoFolder = path;
        emit videoFolderChanged();
    }
}

QString SubtitleAdjustController::subtitleFolder() const { return m_subtitleFolder; }
void SubtitleAdjustController::setSubtitleFolder(const QString &path)
{
    if (m_subtitleFolder != path) {
        m_subtitleFolder = path;
        emit subtitleFolderChanged();
    }
}

bool SubtitleAdjustController::recursiveVideo() const { return m_recursiveVideo; }
void SubtitleAdjustController::setRecursiveVideo(bool recursive)
{
    if (m_recursiveVideo != recursive) {
        m_recursiveVideo = recursive;
        emit recursiveVideoChanged();
    }
}

bool SubtitleAdjustController::recursiveSubtitle() const { return m_recursiveSubtitle; }
void SubtitleAdjustController::setRecursiveSubtitle(bool recursive)
{
    if (m_recursiveSubtitle != recursive) {
        m_recursiveSubtitle = recursive;
        emit recursiveSubtitleChanged();
    }
}

qint64 SubtitleAdjustController::offsetMs() const { return m_offsetMs; }
void SubtitleAdjustController::setOffsetMs(qint64 ms)
{
    if (m_offsetMs != ms) {
        m_offsetMs = ms;
        emit offsetMsChanged();
        if (!m_isDirty) {
            setIsDirty(true);
        }
    }
}

QString SubtitleAdjustController::currentSubtitleText() const { return m_currentSubtitleText; }
bool SubtitleAdjustController::isDirty() const { return m_isDirty; }
int SubtitleAdjustController::currentMatchIndex() const { return m_currentMatchIndex; }
QString SubtitleAdjustController::currentVideoPath() const { return m_currentVideoPath; }
QString SubtitleAdjustController::currentSubtitlePath() const { return m_currentSubtitlePath; }

MatchPairModel *SubtitleAdjustController::matchModel() const { return m_matchModel; }

void SubtitleAdjustController::startMatch()
{
    if (!m_logger)
        return;

    if (m_mode == 0) {
        // 单文件模式：直接添加当前选中的文件对
        m_logger->info(QStringLiteral("单文件模式匹配"));
        if (m_videoPath.isEmpty() || m_subtitlePath.isEmpty())
            return;
        QList<MatchPairModel::MatchPair> pairs;
        pairs.append({m_videoPath, m_subtitlePath, 0});
        m_matchModel->setPairs(pairs);
    } else {
        // 批量模式：扫描文件夹，按文件名前缀匹配
        m_logger->info(QStringLiteral("批量匹配视频字幕"));
        // TODO: scan folders, match by filename stem
    }
    emit matchCompleted();
}

void SubtitleAdjustController::startAdjust(int index)
{
    if (m_logger)
        m_logger->info(QStringLiteral("开始调整: index=%1").arg(index));

    if (index >= 0 && index < m_matchModel->rowCount()) {
        const auto &pair = m_matchModel->at(index);
        setCurrentMatchIndex(index);
        setCurrentVideoPath(pair.videoFile);
        setCurrentSubtitlePath(pair.subtitleFile);
        emit videoReady(m_currentVideoPath, m_currentSubtitlePath);
    }

    m_offsetMs = 0;
    emit offsetMsChanged();
    setCurrentSubtitleText(QString());
    setIsDirty(false);
}

void SubtitleAdjustController::exportSubtitle()
{
    if (m_logger)
        m_logger->info(QStringLiteral("导出字幕"));

    if (m_currentMatchIndex >= 0) {
        m_matchModel->setStatus(m_currentMatchIndex, 1);
    }

    setIsDirty(false);
}

void SubtitleAdjustController::shiftForward(qint64 ms)
{
    setOffsetMs(m_offsetMs + ms);
}

void SubtitleAdjustController::shiftBackward(qint64 ms)
{
    setOffsetMs(m_offsetMs - ms);
}

void SubtitleAdjustController::loadVideo(const QString &videoPath, const QString &subtitlePath)
{
    setCurrentVideoPath(videoPath);
    setCurrentSubtitlePath(subtitlePath);
    m_offsetMs = 0;
    emit offsetMsChanged();
    setCurrentSubtitleText(QString());
    setIsDirty(false);
    emit videoReady(videoPath, subtitlePath);
}

void SubtitleAdjustController::reset()
{
    m_offsetMs = 0;
    emit offsetMsChanged();
    setCurrentSubtitleText(QString());
    setIsDirty(false);
    setCurrentMatchIndex(-1);
    setCurrentVideoPath(QString());
    setCurrentSubtitlePath(QString());
    m_matchModel->clear();
}

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
