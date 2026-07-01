#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class CustomSubtitleSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString subtitleDownloadPath READ subtitleDownloadPath WRITE setSubtitleDownloadPath NOTIFY subtitleDownloadPathChanged)
    Q_PROPERTY(QString videoSourcePath READ videoSourcePath WRITE setVideoSourcePath NOTIFY videoSourcePathChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(QString mergedOutputPath READ mergedOutputPath WRITE setMergedOutputPath NOTIFY mergedOutputPathChanged)
    Q_PROPERTY(QString ffmpegPath READ ffmpegPath WRITE setFfmpegPath NOTIFY ffmpegPathChanged)
    Q_PROPERTY(bool gpuAccel READ gpuAccel WRITE setGpuAccel NOTIFY gpuAccelChanged)
    Q_PROPERTY(bool weakMatch READ weakMatch WRITE setWeakMatch NOTIFY weakMatchChanged)
    Q_PROPERTY(QStringList enabledPreprocessors READ enabledPreprocessors WRITE setEnabledPreprocessors NOTIFY enabledPreprocessorsChanged)

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

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

signals:
    void subtitleDownloadPathChanged();
    void videoSourcePathChanged();
    void recursiveChanged();
    void mergedOutputPathChanged();
    void ffmpegPathChanged();
    void gpuAccelChanged();
    void weakMatchChanged();
    void enabledPreprocessorsChanged();

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
