#pragma once

#include <QObject>
#include <QString>

class BatchRenameSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(int fileType READ fileType WRITE setFileType NOTIFY fileTypeChanged)
    Q_PROPERTY(QString customExtension READ customExtension WRITE setCustomExtension NOTIFY customExtensionChanged)
    Q_PROPERTY(int renameMode READ renameMode WRITE setRenameMode NOTIFY renameModeChanged)
    Q_PROPERTY(QString baseName READ baseName WRITE setBaseName NOTIFY baseNameChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString replaceText READ replaceText WRITE setReplaceText NOTIFY replaceTextChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)

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

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

signals:
    void rootPathChanged();
    void fileTypeChanged();
    void customExtensionChanged();
    void renameModeChanged();
    void baseNameChanged();
    void searchTextChanged();
    void replaceTextChanged();
    void recursiveChanged();

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
