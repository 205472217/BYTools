#pragma once

#include <QObject>
#include <QString>

class FileViewSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sourceFolder READ sourceFolder WRITE setSourceFolder NOTIFY sourceFolderChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(int fileType READ fileType WRITE setFileType NOTIFY fileTypeChanged)
    Q_PROPERTY(int sortField READ sortField WRITE setSortField NOTIFY sortFieldChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(int seekStepMs READ seekStepMs WRITE setSeekStepMs NOTIFY seekStepMsChanged)

public:
    explicit FileViewSettings(QObject *parent = nullptr);

    QString sourceFolder() const;
    bool recursive() const;
    int fileType() const;
    int sortField() const;
    bool sortAscending() const;
    int volume() const;
    bool muted() const;
    int seekStepMs() const;

    void setSourceFolder(const QString &path);
    void setRecursive(bool recursive);
    void setFileType(int type);
    void setSortField(int field);
    void setSortAscending(bool ascending);
    void setVolume(int vol);
    void setMuted(bool m);
    void setSeekStepMs(int ms);

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

signals:
    void sourceFolderChanged();
    void recursiveChanged();
    void fileTypeChanged();
    void sortFieldChanged();
    void sortAscendingChanged();
    void volumeChanged();
    void mutedChanged();
    void seekStepMsChanged();

private:
    QString m_sourceFolder;
    bool m_recursive = false;
    int m_fileType = 0;
    int m_sortField = 0;
    bool m_sortAscending = true;
    int m_volume = 100;
    bool m_muted = false;
    int m_seekStepMs = 5000;
};
