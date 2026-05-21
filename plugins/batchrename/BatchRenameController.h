#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QDir>

class BatchRenameController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(int fileType READ fileType WRITE setFileType NOTIFY fileTypeChanged)
    Q_PROPERTY(QString customExtension READ customExtension WRITE setCustomExtension NOTIFY customExtensionChanged)
    Q_PROPERTY(QString fileTips READ fileTips NOTIFY fileTipsChanged)
    Q_PROPERTY(int renameMode READ renameMode WRITE setRenameMode NOTIFY renameModeChanged)
    Q_PROPERTY(QString baseName READ baseName WRITE setBaseName NOTIFY baseNameChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString replaceText READ replaceText WRITE setReplaceText NOTIFY replaceTextChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)

public:
    explicit BatchRenameController(QObject *parent = nullptr);

    QString rootPath() const;
    void setRootPath(const QString &rootPath);

    int fileType() const;
    void setFileType(int fileType);

    QString customExtension() const;
    void setCustomExtension(const QString &customExtension);

    QString fileTips() const;

    int renameMode() const;
    void setRenameMode(int renameMode);

    QString baseName() const;
    void setBaseName(const QString &baseName);

    QString searchText() const;
    void setSearchText(const QString &searchText);

    QString replaceText() const;
    void setReplaceText(const QString &replaceText);

    bool recursive() const;
    void setRecursive(bool recursive);

    QString statusMessage() const;
    bool hasRecords() const;
    QVariantList records() const;

    Q_INVOKABLE void executeRename();
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void restoreRecord(int index);
    Q_INVOKABLE void restoreAllRecords();
    Q_INVOKABLE void reset();

    void processDirectory(const QDir &currentDir, int &successCount, int &failCount);
    QString getFileType(const QString &fileName) const;

signals:
    void rootPathChanged();
    void fileTypeChanged();
    void customExtensionChanged();
    void fileTipsChanged();
    void renameModeChanged();
    void baseNameChanged();
    void searchTextChanged();
    void replaceTextChanged();
    void recursiveChanged();
    void statusMessageChanged();
    void hasRecordsChanged();
    void recordsChanged();

private:
    struct RenameRecord {
        QString originalPath;
        QString newPath;
        QString originalName;
        QString newName;
        QString fileType;
        bool success;
        QString status;
    };

    void updateFileTips();
    void setStatusMessage(const QString &message);
    void addRecord(const QString &originalPath, const QString &newPath, bool success, const QString &status);
    bool matchesFileType(const QString &fileName) const;
    QString generateNewName(int index, const QString &originalName, const QString &extension) const;
    QString getFileExtension(const QString &fileName) const;

    QString m_rootPath;
    int m_fileType = 0;
    QString m_customExtension;
    QString m_fileTips;
    int m_renameMode = 0;
    QString m_baseName;
    QString m_searchText;
    QString m_replaceText;
    bool m_recursive = false;
    QString m_statusMessage;
    QList<RenameRecord> m_records;

    const QStringList m_videoExtensions = {".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".ts"};
    const QStringList m_audioExtensions = {".mp3", ".wav", ".flac", ".ogg", ".aac", ".wma"};
    const QStringList m_textExtensions = {".txt", ".md", ".json", ".xml", ".csv", ".log"};
    const QStringList m_imageExtensions = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp"};
};