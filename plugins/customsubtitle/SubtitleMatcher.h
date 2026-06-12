#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QPair>

class PluginLogger;

class SubtitleMatcher : public QObject
{
    Q_OBJECT
public:
    explicit SubtitleMatcher(PluginLogger *logger, QObject *parent = nullptr);

    /// Extract key code from filename, e.g. "aaa-304" → "aaa-304", "abc123" → "abc-123"
    static QString extractKey(const QString &fileName);

    /// Result of a single match operation
    struct MatchResult {
        QString subtitlePath;       // original subtitle path
        QString subtitleName;       // original subtitle filename
        QString videoDir;           // target video directory
        QString newSubtitleName;    // renamed subtitle filename
        bool matched = false;
    };

    /// Run matching: scan subtitle dir and video dir, match by key code
    /// @param subtitleDir   Where downloaded .srt files are
    /// @param videoDir      Where video files are
    /// @param recursive     Whether to scan video dir recursively
    /// @param videoExts     List of video file extensions to match
    /// @return List of match results (only matched ones)
    QList<MatchResult> matchSubtitles(const QString &subtitleDir,
                                       const QString &videoDir,
                                       bool recursive,
                                       const QStringList &videoExts);

    /// Execute rename: rename subtitle files in place
    /// @param results  Match results to act upon
    /// @return Number of successfully renamed files
    int executeRename(QList<MatchResult> &results);

    /// Execute move: move renamed subtitles to video directories
    /// @param results  Match results to act upon
    /// @return Number of successfully moved files
    int executeMove(const QList<MatchResult> &results);

signals:
    void logMessage(const QString &message);
    void progress(double value);
    void finished(bool success, const QString &error);

private:
    PluginLogger *m_logger;
};
