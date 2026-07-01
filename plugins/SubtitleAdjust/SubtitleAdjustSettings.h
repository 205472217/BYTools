#pragma once

#include <QObject>
#include <QString>

class SubtitleAdjustSettings : public QObject
{
    Q_OBJECT
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

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

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
