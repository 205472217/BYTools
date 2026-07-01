#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QFileInfo>
#include <QDateTime>

class FileListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    struct FileEntry {
        QString fileName;
        QString filePath;
        qint64 fileSize = 0;
        QDateTime createdTime;
        QDateTime modifiedTime;
        QString fileType;
        int typeCategory = 0; // 0=video, 1=audio, 2=image, 3=document
        bool isDir = false;
        QString thumbnailPath;
    };

    enum Roles {
        FileNameRole = Qt::UserRole + 1,
        FilePathRole,
        FileSizeRole,
        FileSizeDisplayRole,
        CreatedTimeRole,
        CreatedTimeDisplayRole,
        ModifiedTimeRole,
        ModifiedTimeDisplayRole,
        FileTypeRole,
        TypeCategoryRole,
        IsDirRole,
        ThumbnailPathRole
    };

    explicit FileListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFiles(const QList<FileEntry> &files);
    const FileEntry &at(int row) const;
    void clear();

    void setThumbnailPath(int row, const QString &path);

    static QString formatFileSize(qint64 bytes);
    static QString typeCategoryName(int category);

signals:
    void countChanged();

private:
    QList<FileEntry> m_files;
};
