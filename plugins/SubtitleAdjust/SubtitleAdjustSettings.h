#pragma once

#include <QObject>
#include <QString>

class SubtitleAdjustSettings : public QObject
{
    Q_OBJECT

public:
    explicit SubtitleAdjustSettings(QObject *parent = nullptr);

    int mode() const;
    QString videoFolder() const;
    QString subtitleFolder() const;
    bool recursiveVideo() const;
    bool recursiveSubtitle() const;
    bool overwriteOriginal() const;
    int volume() const;
    bool muted() const;
    int seekStepMs() const;

    void setMode(int mode);
    void setVideoFolder(const QString &path);
    void setSubtitleFolder(const QString &path);
    void setRecursiveVideo(bool recursive);
    void setRecursiveSubtitle(bool recursive);
    void setOverwriteOriginal(bool overwrite);
    void setVolume(int vol);
    void setMuted(bool m);
    void setSeekStepMs(int ms);

    // ── 其他功能函数 ──
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

private:
    int m_mode = 0;
    QString m_videoFolder;
    QString m_subtitleFolder;
    bool m_recursiveVideo = false;
    bool m_recursiveSubtitle = false;
    bool m_overwriteOriginal = false;
    int m_volume = 100;
    bool m_muted = false;
    int m_seekStepMs = 5000;
};
