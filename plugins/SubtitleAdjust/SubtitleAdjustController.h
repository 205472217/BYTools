#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QThread>
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
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(int currentMatchIndex READ currentMatchIndex NOTIFY currentMatchIndexChanged)
    Q_PROPERTY(QString currentSubtitleText READ currentSubtitleText NOTIFY currentSubtitleTextChanged)
    Q_PROPERTY(QString currentVideoPath READ currentVideoPath NOTIFY currentVideoPathChanged)
    Q_PROPERTY(QString currentSubtitlePath READ currentSubtitlePath NOTIFY currentSubtitlePathChanged)

    // === 映射表模型 ===
    Q_PROPERTY(MatchPairModel* matchModel READ matchModel CONSTANT)

    // === Config properties (delegated to SubtitleAdjustSettings) ===
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString videoFolder READ videoFolder WRITE setVideoFolder NOTIFY videoFolderChanged)
    Q_PROPERTY(QString subtitleFolder READ subtitleFolder WRITE setSubtitleFolder NOTIFY subtitleFolderChanged)
    Q_PROPERTY(bool recursiveVideo READ recursiveVideo WRITE setRecursiveVideo NOTIFY recursiveVideoChanged)
    Q_PROPERTY(bool recursiveSubtitle READ recursiveSubtitle WRITE setRecursiveSubtitle NOTIFY recursiveSubtitleChanged)
    Q_PROPERTY(bool overwriteOriginal READ overwriteOriginal WRITE setOverwriteOriginal NOTIFY overwriteOriginalChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(int seekStepMs READ seekStepMs WRITE setSeekStepMs NOTIFY seekStepMsChanged)

public:
    struct SubtitleEntry {
        qint64 startTime = 0;   // ms
        qint64 endTime = 0;     // ms
        QString text;
    };

    explicit SubtitleAdjustController(PluginLogger *logger, SubtitleAdjustSettings *settings, QObject *parent = nullptr);
    ~SubtitleAdjustController() override;

    // ── Config properties (delegated to Settings) ──
    int mode() const;
    void setMode(int mode);
    QString videoFolder() const;
    void setVideoFolder(const QString &path);
    QString subtitleFolder() const;
    void setSubtitleFolder(const QString &path);
    bool recursiveVideo() const;
    void setRecursiveVideo(bool recursive);
    bool recursiveSubtitle() const;
    void setRecursiveSubtitle(bool recursive);
    bool overwriteOriginal() const;
    void setOverwriteOriginal(bool overwrite);
    int volume() const;
    void setVolume(int vol);
    bool muted() const;
    void setMuted(bool m);
    int seekStepMs() const;
    void setSeekStepMs(int ms);

    // ── 单文件模式 ──
    QString videoPath() const;
    void setVideoPath(const QString &path);
    QString subtitlePath() const;
    void setSubtitlePath(const QString &path);

    // ── 调整状态 ──
    qint64 offsetMs() const;
    QString currentSubtitleText() const;
    bool isDirty() const;
    bool isProcessing() const;
    int currentMatchIndex() const;
    QString currentVideoPath() const;
    QString currentSubtitlePath() const;

    // ── 模型 ──
    MatchPairModel *matchModel() const;

    // ── Q_INVOKABLE ──
    Q_INVOKABLE void startMatch();
    Q_INVOKABLE void startAdjust(int index);
    Q_INVOKABLE void exportSubtitle();

    Q_INVOKABLE void cancel();
    Q_INVOKABLE void shiftForward(qint64 ms);
    Q_INVOKABLE void shiftBackward(qint64 ms);
    Q_INVOKABLE void setOffsetMs(qint64 ms);

    Q_INVOKABLE void loadVideo(const QString &videoPath, const QString &subtitlePath);
    Q_INVOKABLE void reset();

    Q_INVOKABLE QString getSubtitleTextAt(qint64 positionMs);

signals:
    void modeChanged();
    void videoFolderChanged();
    void subtitleFolderChanged();
    void recursiveVideoChanged();
    void recursiveSubtitleChanged();
    void overwriteOriginalChanged();
    void volumeChanged();
    void mutedChanged();
    void seekStepMsChanged();
    void logMessage(const QString &message);
    void videoPathChanged();
    void subtitlePathChanged();
    void offsetMsChanged();
    void isDirtyChanged();
    void isProcessingChanged();
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
    void doMatchWork();

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

    QThread m_workerThread;
    bool m_workerRunning = false;

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
