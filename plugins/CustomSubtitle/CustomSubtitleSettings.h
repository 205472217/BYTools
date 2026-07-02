#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class CustomSubtitleSettings : public QObject
{
    Q_OBJECT

public:
    explicit CustomSubtitleSettings(QObject *parent = nullptr);

    QString subtitleDownloadPath() const;
    QString videoSourcePath() const;
    bool recursive() const;
    QString mergedOutputPath() const;
    QString ffmpegPath() const;
    bool gpuAccel() const;
    bool weakMatch() const;
    QStringList enabledPreprocessors() const;

    void setSubtitleDownloadPath(const QString &path);
    void setVideoSourcePath(const QString &path);
    void setRecursive(bool recursive);
    void setMergedOutputPath(const QString &path);
    void setFfmpegPath(const QString &path);
    void setGpuAccel(bool enable);
    void setWeakMatch(bool weak);
    void setEnabledPreprocessors(const QStringList &ops);

    // ── 其他功能函数 ──
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

private:
    QString m_subtitleDownloadPath;
    QString m_videoSourcePath;
    bool m_recursive = false;
    QString m_mergedOutputPath;
    QString m_ffmpegPath;
    bool m_gpuAccel = false;
    bool m_weakMatch = false;
    QStringList m_enabledPreprocessors;
};
