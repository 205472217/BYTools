#pragma once

#include <QObject>
#include <QString>

class FileViewSettings : public QObject
{
    Q_OBJECT

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
    int viewWay() const;
    int viewMode() const;

    void setSourceFolder(const QString &path);
    void setRecursive(bool recursive);
    void setFileType(int type);
    void setSortField(int field);
    void setSortAscending(bool ascending);
    void setVolume(int vol);
    void setMuted(bool m);
    void setSeekStepMs(int ms);
    void setViewWay(int mode);
    void setViewMode(int mode);

    // ── 其他功能函数 ──
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

private:
    QString m_sourceFolder;
    bool m_recursive = false;
    int m_fileType = 0;
    int m_sortField = 0;
    bool m_sortAscending = true;
    int m_volume = 100;
    bool m_muted = false;
    int m_seekStepMs = 5000;
    int m_viewWay = 0;
    int m_viewMode = 0;
};
