#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QThread>
#include <QVariantMap>
#include "FileListModel.h"

class PluginLogger;
class FileViewSettings;
class VideoThumbnailGenerator;

class FileViewController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString sourceFolder READ sourceFolder WRITE setSourceFolder NOTIFY sourceFolderChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(int fileType READ fileType WRITE setFileType NOTIFY fileTypeChanged)
    Q_PROPERTY(int sortField READ sortField WRITE setSortField NOTIFY sortFieldChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(int fileCount READ fileCount NOTIFY fileCountChanged)
    Q_PROPERTY(FileListModel* fileListModel READ fileListModel CONSTANT)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentFilePathChanged)
    Q_PROPERTY(QVariantMap currentFileInfo READ currentFileInfo NOTIFY currentFileInfoChanged)
    Q_PROPERTY(int currentModelIndex READ currentModelIndex NOTIFY currentModelIndexChanged)
    Q_PROPERTY(int viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(QString gridCurrentPath READ gridCurrentPath NOTIFY gridCurrentPathChanged)
    Q_PROPERTY(bool canNavigateUp READ canNavigateUp NOTIFY canNavigateUpChanged)

public:
    enum FileType { Video = 0, Audio, Image };
    Q_ENUM(FileType)

    enum SortField { SortName = 0, SortModified, SortCreated, SortSize, SortType };
    Q_ENUM(SortField)

    explicit FileViewController(PluginLogger *logger, FileViewSettings *settings, QObject *parent = nullptr);
    ~FileViewController() override;

    QString sourceFolder() const;
    bool recursive() const;
    int fileType() const;
    int sortField() const;
    bool sortAscending() const;
    bool isProcessing() const;
    int fileCount() const;
    FileListModel *fileListModel() const;
    QString currentFilePath() const;
    QVariantMap currentFileInfo() const;
    int currentModelIndex() const;
    int viewMode() const;
    QString gridCurrentPath() const;
    bool canNavigateUp() const;

    void setSourceFolder(const QString &path);
    void setRecursive(bool recursive);
    void setFileType(int type);
    void setSortField(int field);
    void setSortAscending(bool ascending);
    void setViewMode(int mode);

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void selectFile(int index);
    Q_INVOKABLE void reset();
    Q_INVOKABLE bool deleteFile(int index);
    Q_INVOKABLE bool restoreFile(int index);
    Q_INVOKABLE void cleanTrash();
    Q_INVOKABLE void navigateToDir(const QString &path);
    Q_INVOKABLE void navigateUp();

    static QStringList extensionsForType(int fileType);
    static int categoryForExtension(const QString &ext);

signals:
    void logMessage(const QString &message);
    void sourceFolderChanged();
    void recursiveChanged();
    void fileTypeChanged();
    void sortFieldChanged();
    void sortAscendingChanged();
    void isProcessingChanged();
    void fileCountChanged();
    void currentFilePathChanged();
    void currentFileInfoChanged();
    void currentModelIndexChanged();
    void scanFinished();
    void viewModeChanged();
    void gridCurrentPathChanged();
    void canNavigateUpChanged();

private:
    void doScanWork();
    void scanListFiles();
    void scanGridDirectory();
    void triggerScan();
    void applySort();
    void setCurrentFilePath(const QString &path);
    void setCurrentFileInfo(const QVariantMap &info);
    void setCurrentModelIndex(int index);
    void setGridCurrentPath(const QString &path);

    void startThumbnailGeneration();
    void onThumbnailReady(const QString &filePath, const QString &thumbnailPath);

    PluginLogger *m_logger = nullptr;
    FileViewSettings *m_settings = nullptr;
    VideoThumbnailGenerator *m_thumbnailGenerator = nullptr;

    QThread m_workerThread;
    bool m_workerRunning = false;

    QString m_sourceFolder;
    bool m_recursive = false;
    int m_fileType = 0;
    int m_sortField = 0;
    bool m_sortAscending = true;

    FileListModel *m_fileListModel = nullptr;
    QList<FileListModel::FileEntry> m_allEntries;
    QString m_currentFilePath;
    QVariantMap m_currentFileInfo;
    int m_currentModelIndex = -1;
    int m_viewMode = 1;
    QString m_gridCurrentPath;
    bool m_canNavigateUp = false;
};
