#pragma once

#include <QObject>
#include <QString>

class BatchRenameSettings : public QObject
{
    Q_OBJECT

public:
    explicit BatchRenameSettings(QObject *parent = nullptr);

    QString rootPath() const;
    int fileType() const;
    QString customExtension() const;
    int renameMode() const;
    QString baseName() const;
    QString searchText() const;
    QString replaceText() const;
    bool recursive() const;

    void setRootPath(const QString &path);
    void setFileType(int fileType);
    void setCustomExtension(const QString &ext);
    void setRenameMode(int mode);
    void setBaseName(const QString &name);
    void setSearchText(const QString &text);
    void setReplaceText(const QString &text);
    void setRecursive(bool recursive);

    // ── 其他功能函数 ──
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

private:
    QString m_rootPath;
    int m_fileType = 0;
    QString m_customExtension;
    int m_renameMode = 0;
    QString m_baseName;
    QString m_searchText;
    QString m_replaceText;
    bool m_recursive = false;
};
