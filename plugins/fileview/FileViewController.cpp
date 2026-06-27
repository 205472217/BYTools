#include "FileViewController.h"
#include "FileViewSettings.h"
#include "Logger.h"
#include "Config.h"
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QCollator>
#include <algorithm>
#include <functional>

static QVariantMap emptyFileInfo()
{
    QVariantMap info;
    info["fileName"] = QStringLiteral("");
    info["filePath"] = QStringLiteral("");
    info["fileSize"] = 0;
    info["fileSizeDisplay"] = QStringLiteral("");
    info["createdTimeDisplay"] = QStringLiteral("");
    info["modifiedTimeDisplay"] = QStringLiteral("");
    info["fileType"] = QStringLiteral("");
    info["typeCategory"] = -1;
    info["typeCategoryName"] = QStringLiteral("");
    return info;
}

FileViewController::FileViewController(PluginLogger *logger, FileViewSettings *settings, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(settings)
{
    m_fileListModel = new FileListModel(this);
    setCurrentFileInfo(emptyFileInfo());

    connect(&m_workerThread, &QThread::started,
            this, &FileViewController::doScanWork, Qt::DirectConnection);
    connect(&m_workerThread, &QThread::finished, this, [this]() {
        m_workerRunning = false;
        emit isProcessingChanged();
        emit scanFinished();
    });
}

FileViewController::~FileViewController()
{
    if (m_workerRunning) {
        m_workerThread.quit();
        m_workerThread.wait(5000);
    }
}

QString FileViewController::sourceFolder() const { return m_sourceFolder; }
bool FileViewController::recursive() const { return m_recursive; }
int FileViewController::fileType() const { return m_fileType; }
int FileViewController::sortField() const { return m_sortField; }
bool FileViewController::sortAscending() const { return m_sortAscending; }
bool FileViewController::isProcessing() const { return m_workerRunning; }
int FileViewController::fileCount() const { return m_fileListModel->rowCount(); }
FileListModel *FileViewController::fileListModel() const { return m_fileListModel; }
QString FileViewController::currentFilePath() const { return m_currentFilePath; }
QVariantMap FileViewController::currentFileInfo() const { return m_currentFileInfo; }
int FileViewController::currentModelIndex() const { return m_currentModelIndex; }

void FileViewController::setSourceFolder(const QString &path)
{
    if (m_sourceFolder != path) {
        m_sourceFolder = path;
        m_settings->setSourceFolder(path);
        emit sourceFolderChanged();
    }
}

void FileViewController::setRecursive(bool recursive)
{
    if (m_recursive != recursive) {
        m_recursive = recursive;
        m_settings->setRecursive(recursive);
        emit recursiveChanged();
    }
}

void FileViewController::setFileType(int type)
{
    if (m_fileType != type) {
        m_fileType = type;
        m_settings->setFileType(type);
        emit fileTypeChanged();
    }
}

void FileViewController::setSortField(int field)
{
    if (m_sortField != field) {
        m_sortField = field;
        m_settings->setSortField(field);
        emit sortFieldChanged();
        applySort();
    }
}

void FileViewController::setSortAscending(bool ascending)
{
    if (m_sortAscending != ascending) {
        m_sortAscending = ascending;
        m_settings->setSortAscending(ascending);
        emit sortAscendingChanged();
        applySort();
    }
}

// ── 扩展名列表 ──

QStringList FileViewController::extensionsForType(int fileType)
{
    switch (fileType) {
    case Video:
        return {"*.mp4", "*.mkv", "*.avi", "*.mov", "*.wmv", "*.flv", "*.webm", "*.m4v", "*.ts", "*.rmvb"};
    case Audio:
        return {"*.mp3", "*.wav", "*.flac", "*.aac", "*.ogg", "*.wma", "*.m4a", "*.opus"};
    case Image:
        return {"*.jpg", "*.jpeg", "*.png", "*.gif", "*.bmp", "*.webp", "*.svg", "*.tiff", "*.tif", "*.ico"};
    case Document:
        return {"*.txt", "*.pdf", "*.doc", "*.docx", "*.xls", "*.xlsx", "*.ppt", "*.pptx", "*.md", "*.epub", "*.csv"};
    }
    return {};
}

int FileViewController::categoryForExtension(const QString &ext)
{
    const QString lower = ext.toLower();
    static const QStringList videoExts = {".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".ts", ".rmvb"};
    static const QStringList audioExts = {".mp3", ".wav", ".flac", ".aac", ".ogg", ".wma", ".m4a", ".opus"};
    static const QStringList imageExts = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".svg", ".tiff", ".tif", ".ico"};
    static const QStringList docExts  = {".txt", ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", ".md", ".epub", ".csv"};

    if (videoExts.contains(lower)) return Video;
    if (audioExts.contains(lower)) return Audio;
    if (imageExts.contains(lower)) return Image;
    if (docExts.contains(lower))  return Document;
    return -1;
}

// ── 扫描 ──

void FileViewController::startScan()
{
    if (!m_logger)
        return;

    if (m_sourceFolder.isEmpty()) {
        QString msg = QStringLiteral("请先选择源文件夹");
        m_logger->warn(msg);
        emit logMessage(msg);
        return;
    }

    if (!QFileInfo::exists(m_sourceFolder)) {
        QString msg = QStringLiteral("文件夹不存在: ") + m_sourceFolder;
        m_logger->warn(msg);
        emit logMessage(msg);
        return;
    }

    m_allEntries.clear();
    m_fileListModel->clear();
    setCurrentFilePath(QString());
    setCurrentFileInfo(emptyFileInfo());

    emit logMessage(QStringLiteral("正在扫描文件夹..."));
    if (m_recursive)
        emit logMessage("  启用了递归查找，正在遍历子目录...");

    m_workerRunning = true;
    emit isProcessingChanged();
    m_workerThread.start();
}

void FileViewController::doScanWork()
{
    QStringList exts = extensionsForType(m_fileType);

    QList<FileListModel::FileEntry> entries;

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (m_recursive)
        flags |= QDirIterator::Subdirectories;

    QDirIterator it(m_sourceFolder, exts, QDir::Files, flags);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();

        FileListModel::FileEntry entry;
        entry.fileName = fi.fileName();
        entry.filePath = fi.absoluteFilePath();
        entry.fileSize = fi.size();
        entry.createdTime = fi.birthTime();
        entry.modifiedTime = fi.lastModified();
        entry.fileType = fi.suffix().toLower();
        entry.typeCategory = m_fileType;
        entries.append(entry);
    }

    m_workerThread.quit();

    QMetaObject::invokeMethod(this, [this, entries]() {
        m_allEntries = entries;
        applySort();

        QString msg = QStringLiteral("✓ 扫描完成: 共找到 %1 个文件")
            .arg(entries.size());
        m_logger->info(msg);
        emit logMessage(msg);

        if (entries.isEmpty())
            emit logMessage(QStringLiteral("⚠ 未找到匹配的文件，请检查文件夹路径或文件类型"));
    }, Qt::QueuedConnection);
}

// ── 排序 ──

void FileViewController::applySort()
{
    QCollator c;
    c.setNumericMode(true);

    std::sort(m_allEntries.begin(), m_allEntries.end(),
              [this, &c](const FileListModel::FileEntry &a, const FileListModel::FileEntry &b) {
        int cmp = 0;
        switch (m_sortField) {
        case SortName:
            cmp = c.compare(a.fileName, b.fileName);
            break;
        case SortModified:
            if (a.modifiedTime < b.modifiedTime) cmp = -1;
            else if (a.modifiedTime > b.modifiedTime) cmp = 1;
            break;
        case SortCreated:
            if (a.createdTime < b.createdTime) cmp = -1;
            else if (a.createdTime > b.createdTime) cmp = 1;
            break;
        case SortSize:
            if (a.fileSize < b.fileSize) cmp = -1;
            else if (a.fileSize > b.fileSize) cmp = 1;
            break;
        case SortType:
            cmp = c.compare(a.fileType, b.fileType);
            if (cmp == 0)
                cmp = c.compare(a.fileName, b.fileName);
            break;
        }
        return m_sortAscending ? cmp < 0 : cmp > 0;
    });

    m_fileListModel->setFiles(m_allEntries);
    emit fileCountChanged();

    // 排序后恢复当前选中项
    if (!m_currentFilePath.isEmpty()) {
        for (int i = 0; i < m_allEntries.size(); ++i) {
            if (m_allEntries[i].filePath == m_currentFilePath) {
                selectFile(i);
                return;
            }
        }
    }
    setCurrentModelIndex(-1);
}

// ── 选择文件 ──

void FileViewController::selectFile(int index)
{
    if (index < 0 || index >= m_allEntries.size()) {
        setCurrentFilePath(QString());
        setCurrentFileInfo(emptyFileInfo());
        setCurrentModelIndex(-1);
        return;
    }

    const auto &entry = m_allEntries.at(index);

    setCurrentFilePath(entry.filePath);
    setCurrentModelIndex(index);

    QVariantMap info;
    info["fileName"] = entry.fileName;
    info["filePath"] = entry.filePath;
    info["fileSize"] = entry.fileSize;
    info["fileSizeDisplay"] = FileListModel::formatFileSize(entry.fileSize);
    info["createdTime"] = entry.createdTime;
    info["createdTimeDisplay"] = entry.createdTime.isValid()
        ? entry.createdTime.toString("yyyy-MM-dd HH:mm:ss") : QStringLiteral("");
    info["modifiedTime"] = entry.modifiedTime;
    info["modifiedTimeDisplay"] = entry.modifiedTime.isValid()
        ? entry.modifiedTime.toString("yyyy-MM-dd HH:mm:ss") : QStringLiteral("");
    info["fileType"] = entry.fileType;
    info["typeCategory"] = entry.typeCategory;
    info["typeCategoryName"] = FileListModel::typeCategoryName(entry.typeCategory);
    setCurrentFileInfo(info);
}

void FileViewController::cancel()
{
    if (m_workerRunning) {
        m_workerThread.quit();
        m_workerThread.wait(500);
        m_workerRunning = false;
        emit isProcessingChanged();
    }
}

void FileViewController::reset()
{
    cancel();
    m_allEntries.clear();
    m_fileListModel->clear();
    setCurrentFilePath(QString());

    // 恢复上次设置
    m_sourceFolder = m_settings->sourceFolder();
    m_fileType = m_settings->fileType();
    m_recursive = m_settings->recursive();
    m_sortField = m_settings->sortField();
    m_sortAscending = m_settings->sortAscending();

    setCurrentFileInfo(emptyFileInfo());

    emit sourceFolderChanged();
    emit fileTypeChanged();
    emit recursiveChanged();
    emit sortFieldChanged();
    emit sortAscendingChanged();
    emit fileCountChanged();
}

void FileViewController::setCurrentFilePath(const QString &path)
{
    if (m_currentFilePath != path) {
        m_currentFilePath = path;
        emit currentFilePathChanged();
    }
}

void FileViewController::setCurrentFileInfo(const QVariantMap &info)
{
    m_currentFileInfo = info;
    emit currentFileInfoChanged();
}

void FileViewController::setCurrentModelIndex(int index)
{
    if (m_currentModelIndex != index) {
        m_currentModelIndex = index;
        emit currentModelIndexChanged();
    }
}
