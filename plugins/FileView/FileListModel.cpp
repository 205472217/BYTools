#include "FileListModel.h"
#include <QFileInfo>

FileListModel::FileListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FileListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_files.size();
}

QVariant FileListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_files.size())
        return {};

    const auto &entry = m_files.at(index.row());

    switch (role) {
    case FileNameRole:
        return entry.fileName;
    case FilePathRole:
        return entry.filePath;
    case FileSizeRole:
        return entry.fileSize;
    case FileSizeDisplayRole:
        return formatFileSize(entry.fileSize);
    case CreatedTimeRole:
        return entry.createdTime;
    case CreatedTimeDisplayRole:
        return entry.createdTime.isValid()
            ? entry.createdTime.toString("yyyy-MM-dd HH:mm:ss")
            : QStringLiteral("");
    case ModifiedTimeRole:
        return entry.modifiedTime;
    case ModifiedTimeDisplayRole:
        return entry.modifiedTime.isValid()
            ? entry.modifiedTime.toString("yyyy-MM-dd HH:mm:ss")
            : QStringLiteral("");
    case FileTypeRole:
        return entry.fileType;
    case TypeCategoryRole:
        return entry.typeCategory;
    case IsDirRole:
        return entry.isDir;
    case ThumbnailPathRole:
        return entry.thumbnailPath;
    }
    return {};
}

QHash<int, QByteArray> FileListModel::roleNames() const
{
    return {
        { FileNameRole,         "fileName" },
        { FilePathRole,         "filePath" },
        { FileSizeRole,         "fileSize" },
        { FileSizeDisplayRole,  "fileSizeDisplay" },
        { CreatedTimeRole,      "createdTime" },
        { CreatedTimeDisplayRole, "createdTimeDisplay" },
        { ModifiedTimeRole,     "modifiedTime" },
        { ModifiedTimeDisplayRole, "modifiedTimeDisplay" },
        { FileTypeRole,         "fileType" },
        { TypeCategoryRole,     "typeCategory" },
        { IsDirRole,            "isDir" },
        { ThumbnailPathRole,    "thumbnailPath" },
    };
}

void FileListModel::setThumbnailPath(int row, const QString &path)
{
    if (row < 0 || row >= m_files.size())
        return;
    m_files[row].thumbnailPath = path;
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ThumbnailPathRole});
}

void FileListModel::setFiles(const QList<FileEntry> &files)
{
    beginResetModel();
    m_files = files;
    endResetModel();
    emit countChanged();
}

const FileListModel::FileEntry &FileListModel::at(int row) const
{
    return m_files.at(row);
}

void FileListModel::clear()
{
    beginResetModel();
    m_files.clear();
    endResetModel();
    emit countChanged();
}

QString FileListModel::formatFileSize(qint64 bytes)
{
    if (bytes < 1024)
        return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024)
        return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024LL * 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

QString FileListModel::typeCategoryName(int category)
{
    switch (category) {
    case 0: return QStringLiteral("视频");
    case 1: return QStringLiteral("音频");
    case 2: return QStringLiteral("图片");
    case 3: return QStringLiteral("文档");
    default: return QStringLiteral("");
    }
}
