#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include "MatchPairModel.h"

class PluginLogger;
class SubtitleAdjustSettings;

class SubtitleAdjustController : public QObject
{
    Q_OBJECT

    // ── 单文件模式 ──
    Q_PROPERTY(QString videoPath READ videoPath WRITE setVideoPath NOTIFY videoPathChanged)
    Q_PROPERTY(QString subtitlePath READ subtitlePath WRITE setSubtitlePath NOTIFY subtitlePathChanged)

    // ── 调整状态 ──
    Q_PROPERTY(qint64 offsetMs READ offsetMs WRITE setOffsetMs NOTIFY offsetMsChanged)
    Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)
    Q_PROPERTY(int currentMatchIndex READ currentMatchIndex NOTIFY currentMatchIndexChanged)
    Q_PROPERTY(QString currentSubtitleText READ currentSubtitleText NOTIFY currentSubtitleTextChanged)
    Q_PROPERTY(QString currentVideoPath READ currentVideoPath NOTIFY currentVideoPathChanged)
    Q_PROPERTY(QString currentSubtitlePath READ currentSubtitlePath NOTIFY currentSubtitlePathChanged)

    // === 映射表模型 ===
    Q_PROPERTY(MatchPairModel* matchModel READ matchModel CONSTANT)

public:
    struct SubtitleEntry {
        qint64 startTime = 0;   // ms
        qint64 endTime = 0;     // ms
        QString text;
    };

    explicit SubtitleAdjustController(PluginLogger *logger, SubtitleAdjustSettings *settings, QObject *parent = nullptr);

    // ── Config properties (delegated to Settings) ──
    bool overwriteOriginal() const;
    void setOverwriteOriginal(bool overwrite);

    // ── 单文件模式 ──
    QString videoPath() const;
    void setVideoPath(const QString &path);
    QString subtitlePath() const;
    void setSubtitlePath(const QString &path);

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
    void overwriteOriginalChanged();
    void logMessage(const QString &message);
    void videoPathChanged();
    void subtitlePathChanged();
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

    struct CompletedRecord {
        QString videoPath;
        QString subtitlePath;
        qint64 offsetMs;
        QString timestamp;
    };

    void loadRecords();
    void saveRecord(const QString &videoPath, const QString &subtitlePath, qint64 offsetMs);
    bool hasRecord(const QString &subtitlePath) const;
    QString recordsFilePath() const;

    PluginLogger *m_logger = nullptr;
    SubtitleAdjustSettings *m_settings = nullptr;

    QString m_videoPath;
    QString m_subtitlePath;

    qint64 m_offsetMs = 0;
    bool m_isDirty = false;
    int m_currentMatchIndex = -1;
    QString m_currentSubtitleText;
    QString m_currentVideoPath;
    QString m_currentSubtitlePath;

    MatchPairModel *m_matchModel = nullptr;

    QList<SubtitleEntry> m_subtitleEntries;
    QString m_srtFilePath;

    QHash<QString, CompletedRecord> m_records; // keyed by subtitlePath
};
