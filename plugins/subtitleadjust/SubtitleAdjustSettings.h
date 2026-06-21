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

public:
    explicit SubtitleAdjustSettings(QObject *parent = nullptr);

    int mode() const;
    QString videoFolder() const;
    QString subtitleFolder() const;
    bool recursiveVideo() const;
    bool recursiveSubtitle() const;
    bool overwriteOriginal() const;

    void setMode(int mode);
    void setVideoFolder(const QString &path);
    void setSubtitleFolder(const QString &path);
    void setRecursiveVideo(bool recursive);
    void setRecursiveSubtitle(bool recursive);
    void setOverwriteOriginal(bool overwrite);

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

signals:
    void modeChanged();
    void videoFolderChanged();
    void subtitleFolderChanged();
    void recursiveVideoChanged();
    void recursiveSubtitleChanged();
    void overwriteOriginalChanged();

private:
    int m_mode = 0;
    QString m_videoFolder;
    QString m_subtitleFolder;
    bool m_recursiveVideo = false;
    bool m_recursiveSubtitle = false;
    bool m_overwriteOriginal = false;
};
