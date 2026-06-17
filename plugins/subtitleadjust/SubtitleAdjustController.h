#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include "MatchPairModel.h"

class PluginLogger;

class SubtitleAdjustController : public QObject
{
    Q_OBJECT

    // ── 模式 ──
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)

    // ── 单文件模式 ──
    Q_PROPERTY(QString videoPath READ videoPath WRITE setVideoPath NOTIFY videoPathChanged)
    Q_PROPERTY(QString subtitlePath READ subtitlePath WRITE setSubtitlePath NOTIFY subtitlePathChanged)

    // ── 批量模式 ──
    Q_PROPERTY(QString videoFolder READ videoFolder WRITE setVideoFolder NOTIFY videoFolderChanged)
    Q_PROPERTY(QString subtitleFolder READ subtitleFolder WRITE setSubtitleFolder NOTIFY subtitleFolderChanged)
    Q_PROPERTY(bool recursiveVideo READ recursiveVideo WRITE setRecursiveVideo NOTIFY recursiveVideoChanged)
    Q_PROPERTY(bool recursiveSubtitle READ recursiveSubtitle WRITE setRecursiveSubtitle NOTIFY recursiveSubtitleChanged)

    // ── 调整状态 ──
    Q_PROPERTY(qint64 offsetMs READ offsetMs WRITE setOffsetMs NOTIFY offsetMsChanged)
    Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)
    Q_PROPERTY(int currentMatchIndex READ currentMatchIndex NOTIFY currentMatchIndexChanged)
    Q_PROPERTY(QString currentSubtitleText READ currentSubtitleText NOTIFY currentSubtitleTextChanged)
    Q_PROPERTY(QString currentVideoPath READ currentVideoPath NOTIFY currentVideoPathChanged)
    Q_PROPERTY(QString currentSubtitlePath READ currentSubtitlePath NOTIFY currentSubtitlePathChanged)

    // ── 映射表模型 ──
    Q_PROPERTY(MatchPairModel* matchModel READ matchModel CONSTANT)

public:
    struct SubtitleEntry {
        qint64 startTime = 0;   // ms
        qint64 endTime = 0;     // ms
        QString text;
    };

    explicit SubtitleAdjustController(PluginLogger *logger, QObject *parent = nullptr);

    // ── mode ──
    int mode() const;
    void setMode(int mode);

    // ── 单文件模式 ──
    QString videoPath() const;
    void setVideoPath(const QString &path);
    QString subtitlePath() const;
    void setSubtitlePath(const QString &path);

    // ── 批量模式 ──
    QString videoFolder() const;
    void setVideoFolder(const QString &path);
    QString subtitleFolder() const;
    void setSubtitleFolder(const QString &path);
    bool recursiveVideo() const;
    void setRecursiveVideo(bool recursive);
    bool recursiveSubtitle() const;
    void setRecursiveSubtitle(bool recursive);

    // ── 调整状态 ──
    qint64 offsetMs() const;
    QString currentSubtitleText() const;
    bool isDirty() const;
    int currentMatchIndex() const;
    QString currentVideoPath() const;
    QString currentSubtitlePath() const;

    // ── 模型 ──
    MatchPairModel *matchModel() const;

    // ── Q_INVOKABLE ──
    Q_INVOKABLE void startMatch();
    Q_INVOKABLE void startAdjust(int index);
    Q_INVOKABLE void exportSubtitle();

    Q_INVOKABLE void shiftForward(qint64 ms);
    Q_INVOKABLE void shiftBackward(qint64 ms);
    Q_INVOKABLE void setOffsetMs(qint64 ms);

    Q_INVOKABLE void loadVideo(const QString &videoPath, const QString &subtitlePath);
    Q_INVOKABLE void reset();

    Q_INVOKABLE QString getSubtitleTextAt(qint64 positionMs);

signals:
    void logMessage(const QString &message);
    void modeChanged();
    void videoPathChanged();
    void subtitlePathChanged();
    void videoFolderChanged();
    void subtitleFolderChanged();
    void recursiveVideoChanged();
    void recursiveSubtitleChanged();
    void offsetMsChanged();
    void isDirtyChanged();
    void currentMatchIndexChanged();
    void currentSubtitleTextChanged();
    void currentVideoPathChanged();
    void currentSubtitlePathChanged();
    void matchCompleted();
    void videoReady(const QString &videoPath, const QString &subtitlePath);
    void exportFinished(bool success, const QString &message);

private:
    void setCurrentSubtitleText(const QString &text);
    void setIsDirty(bool dirty);
    void setCurrentMatchIndex(int index);
    void setCurrentVideoPath(const QString &path);
    void setCurrentSubtitlePath(const QString &path);

    bool parseSrtFile(const QString &filePath);
    bool writeAdjustedSrt(const QString &outputPath);
    static qint64 parseSrtTime(const QString &timeStr);
    static QString formatSrtTime(qint64 ms);
    void collectFiles(const QString &dirPath, bool recursive,
                      const QStringList &extensions, QStringList &results);

    PluginLogger *m_logger = nullptr;

    int m_mode = 0;

    QString m_videoPath;
    QString m_subtitlePath;

    QString m_videoFolder;
    QString m_subtitleFolder;
    bool m_recursiveVideo = false;
    bool m_recursiveSubtitle = false;

    qint64 m_offsetMs = 0;
    bool m_isDirty = false;
    int m_currentMatchIndex = -1;
    QString m_currentSubtitleText;
    QString m_currentVideoPath;
    QString m_currentSubtitlePath;

    MatchPairModel *m_matchModel = nullptr;

    QList<SubtitleEntry> m_subtitleEntries;
    QString m_srtFilePath;
};
