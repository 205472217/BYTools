#include "FileViewController.h"
#include "FileViewSettings.h"
#include "VideoThumbnailGenerator.h"
#include "GlobalConfig.h"
#include "Logger.h"
#include "Config.h"
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QImage>
#include <QCollator>
#include <QDateTime>
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
    info["imageResolution"] = QStringLiteral("");
    info["imageBitDepth"] = QStringLiteral("");
    return info;
}

FileViewController::FileViewController(PluginLogger *logger, FileViewSettings *settings, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_settings(settings)
{
    m_fileListModel = new FileListModel(this);
    setCurrentFileInfo(emptyFileInfo());

    QString ffmpegPath = GlobalConfig::instance()->ffmpegPath();
    if (!ffmpegPath.isEmpty()) {
        m_thumbnailGenerator = new VideoThumbnailGenerator(this);
        m_thumbnailGenerator->setFfmpegPath(ffmpegPath);
        connect(m_thumbnailGenerator, &VideoThumbnailGenerator::thumbnailReady,
                this, &FileViewController::onThumbnailReady);
    }

    m_thumbnailDelayTimer = new QTimer(this);
    m_thumbnailDelayTimer->setSingleShot(true);
    m_thumbnailDelayTimer->setInterval(1000);
    connect(m_thumbnailDelayTimer, &QTimer::timeout, this, &FileViewController::startThumbnailGeneration);

    // 从持久化设置同步缓存成员
    m_viewWay = m_settings->viewWay();
    m_viewMode = m_settings->viewMode();

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
    if (m_thumbnailGenerator)
        m_thumbnailGenerator->cancel();
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
int FileViewController::viewWay() const { return m_viewWay; }
int FileViewController::viewMode() const { return m_viewMode; }
QString FileViewController::currentPath() const { return m_currentPath; }
bool FileViewController::canNavigateUp() const { return m_canNavigateUp; }

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

void FileViewController::setViewWay(int mode)
{
    if (m_viewWay == mode)
        return;
    m_viewWay = mode;
    m_settings->setViewWay(mode);
    emit viewWayChanged();
}

void FileViewController::setViewMode(int mode)
{
    if (m_viewMode == mode)
        return;
    m_viewMode = mode;
    m_settings->setViewMode(mode);
    emit viewModeChanged();
}

int FileViewController::volume() const { return m_settings->volume(); }
void FileViewController::setVolume(int vol) {
    if (m_settings->volume() != vol) {
        m_settings->setVolume(vol);
        emit volumeChanged();
    }
}
bool FileViewController::muted() const { return m_settings->muted(); }
void FileViewController::setMuted(bool m) {
    if (m_settings->muted() != m) {
        m_settings->setMuted(m);
        emit mutedChanged();
    }
}
int FileViewController::seekStepMs() const { return m_settings->seekStepMs(); }
void FileViewController::setSeekStepMs(int ms) {
    if (m_settings->seekStepMs() != ms) {
        m_settings->setSeekStepMs(ms);
        emit seekStepMsChanged();
    }
}

// ── 扩展名列表（全局仅定义一次） ──

namespace {

const QStringList kVideoGlob = {
    "*.mp4", "*.mkv", "*.avi", "*.mov", "*.wmv", "*.flv", "*.webm", "*.m4v", "*.ts", "*.rmvb"
};
const QStringList kAudioGlob = {
    "*.mp3", "*.wav", "*.flac", "*.aac", "*.ogg", "*.wma", "*.m4a", "*.opus"
};
const QStringList kImageGlob = {
    "*.jpg", "*.jpeg", "*.png", "*.gif", "*.bmp", "*.webp", "*.svg", "*.tiff", "*.tif", "*.ico"
};

const QStringList kVideoSuffix = {
    ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".ts", ".rmvb"
};
const QStringList kAudioSuffix = {
    ".mp3", ".wav", ".flac", ".aac", ".ogg", ".wma", ".m4a", ".opus"
};
const QStringList kImageSuffix = {
    ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".svg", ".tiff", ".tif", ".ico"
};

}

QStringList FileViewController::extensionsForType(int fileType)
{
    switch (fileType) {
    case Video: return kVideoGlob;
    case Audio: return kAudioGlob;
    case Image: return kImageGlob;
    case All:   return kVideoGlob + kAudioGlob + kImageGlob;
    }
    return {};
}

int FileViewController::categoryForExtension(const QString &ext)
{
    const QString lower = ext.toLower();
    if (kVideoSuffix.contains(lower)) return Video;
    if (kAudioSuffix.contains(lower)) return Audio;
    if (kImageSuffix.contains(lower)) return Image;
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

    m_thumbnailDelayTimer->stop();
    m_pendingThumbnailRequests.clear();
    m_allEntries.clear();
    m_fileListModel->clear();
    setCurrentFilePath(QString());
    setCurrentFileInfo(emptyFileInfo());

    if (m_viewWay == 0) {
        setCurrentPath(m_sourceFolder);
        emit logMessage(QStringLiteral("正在扫描文件夹..."));
    } else {
        emit logMessage(QStringLiteral("正在扫描文件夹..."));
        if (m_recursive)
            emit logMessage("  启用了递归查找，正在遍历子目录...");
    }

    triggerScan();
}

void FileViewController::setCurrentPath(const QString &path)
{
    if (m_currentPath != path) {
        m_currentPath = path;
        emit currentPathChanged();

        bool canUp = m_currentPath != m_sourceFolder
            && m_currentPath.startsWith(m_sourceFolder);
        if (m_canNavigateUp != canUp) {
            m_canNavigateUp = canUp;
            emit canNavigateUpChanged();
        }
    }
}

void FileViewController::triggerScan()
{
    if (m_workerThread.isRunning()) {
        m_workerThread.requestInterruption();
        m_workerThread.quit();
        m_workerThread.wait();
    }
    m_workerRunning = true;
    emit isProcessingChanged();
    m_workerThread.start();
}

void FileViewController::doScanWork()
{
    if (m_viewWay == 0)
        scanDirectorys();
    else
        scanFiles();
}

void FileViewController::scanFiles()
{
    QStringList exts = extensionsForType(m_fileType);

    QList<FileListModel::FileEntry> entries;
    QList<VideoThumbnailGenerator::ThumbnailRequest> videoRequests;

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (m_recursive)
        flags |= QDirIterator::Subdirectories;

    QDirIterator it(m_sourceFolder, exts, QDir::Files, flags);
    while (it.hasNext()) {
        if (QThread::currentThread()->isInterruptionRequested())
            return;
        it.next();
        QFileInfo fi = it.fileInfo();

        static const QStringList folderCoverNames = {
            "folder.jpg", "folder.png", "folder.jpeg", "folder.webp"
        };
        if (folderCoverNames.contains(fi.fileName().toLower()))
            continue;

        FileListModel::FileEntry entry;
        entry.fileName = fi.fileName();
        entry.filePath = fi.absoluteFilePath();
        entry.fileSize = fi.size();
        entry.createdTime = fi.birthTime();
        entry.modifiedTime = fi.lastModified();
        entry.fileType = fi.suffix().toLower();
        entry.typeCategory = (m_fileType == All)
            ? categoryForExtension(QStringLiteral(".") + entry.fileType)
            : m_fileType;
        entries.append(entry);

        if (entry.typeCategory == 0)
            videoRequests.append({entry.filePath, entry.modifiedTime});
    }

    m_workerThread.quit();

    QMetaObject::invokeMethod(this, [this, entries, videoRequests]() {
        m_pendingThumbnailRequests = videoRequests;
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

void FileViewController::scanDirectorys()
{
    QStringList exts = extensionsForType(m_fileType);

    QList<FileListModel::FileEntry> entries;
    QList<VideoThumbnailGenerator::ThumbnailRequest> videoRequests;

    QDir dir(m_currentPath);

    // Scan subdirectories
    QFileInfoList dirInfos = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
    for (const QFileInfo &fi : dirInfos) {
        if (QThread::currentThread()->isInterruptionRequested())
            return;
        FileListModel::FileEntry entry;
        entry.fileName = fi.fileName();
        entry.filePath = fi.absoluteFilePath();
        entry.isDir = true;
        entry.fileSize = fi.size();
        entry.createdTime = fi.birthTime();
        entry.modifiedTime = fi.lastModified();
        entry.fileType = QStringLiteral("文件夹");
        entry.typeCategory = -1;

        static const QStringList folderThumbNames = {
            "folder.jpg", "folder.png", "folder.jpeg", "folder.webp"
        };
        for (const QString &thumbName : folderThumbNames) {
            QString thumbPath = fi.absoluteFilePath() + QStringLiteral("/") + thumbName;
            if (QFileInfo::exists(thumbPath)) {
                entry.thumbnailPath = thumbPath;
                break;
            }
        }

        entries.append(entry);
    }

    // Scan files
    QStringList nameFilters = exts;
    if (nameFilters.isEmpty())
        nameFilters << QStringLiteral("*");
    QFileInfoList fileInfos = dir.entryInfoList(nameFilters, QDir::Files);
    for (const QFileInfo &fi : fileInfos) {
        if (QThread::currentThread()->isInterruptionRequested())
            return;

        // Skip folder cover images (folder.jpg/png/jpeg/webp)
        static const QStringList folderCoverNames = {
            "folder.jpg", "folder.png", "folder.jpeg", "folder.webp"
        };
        if (folderCoverNames.contains(fi.fileName().toLower()))
            continue;

        FileListModel::FileEntry entry;
        entry.fileName = fi.fileName();
        entry.filePath = fi.absoluteFilePath();
        entry.fileSize = fi.size();
        entry.createdTime = fi.birthTime();
        entry.modifiedTime = fi.lastModified();
        entry.fileType = fi.suffix().toLower();
        entry.typeCategory = categoryForExtension(QStringLiteral(".") + entry.fileType);

        if (entry.typeCategory == Image)
            entry.thumbnailPath = fi.absoluteFilePath();

        entries.append(entry);

        if (entry.typeCategory == 0)
            videoRequests.append({entry.filePath, entry.modifiedTime});
    }

    m_workerThread.quit();

    QMetaObject::invokeMethod(this, [this, entries, videoRequests]() {
        m_pendingThumbnailRequests = videoRequests;
        m_allEntries = entries;
        applySort();

        QString msg = QStringLiteral("✓ 扫描完成: 共 %1 项").arg(entries.size());
        m_logger->info(msg);
        emit logMessage(msg);

        if (entries.isEmpty())
            emit logMessage(QStringLiteral("⚠ 当前文件夹为空"));
    }, Qt::QueuedConnection);
}

// ── 排序 ──

void FileViewController::applySort()
{
    QCollator c;
    c.setNumericMode(true);

    std::sort(m_allEntries.begin(), m_allEntries.end(),
              [this, &c](const FileListModel::FileEntry &a, const FileListModel::FileEntry &b) {
        // directories always come first
        if (m_viewWay == 0 && a.isDir != b.isDir)
            return a.isDir;

        int cmp = 0;
        switch (m_sortField) {
        case SortName:
            cmp = c.compare(a.fileName, b.fileName);
            break;
        case SortModified:
            if (a.modifiedTime < b.modifiedTime) cmp = -1;
            else if (a.modifiedTime > b.modifiedTime) cmp = 1;
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

    m_thumbnailDelayTimer->start();

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

    if (entry.typeCategory == 2) {
        QImage img(entry.filePath);
        if (!img.isNull()) {
            info["imageResolution"] = QStringLiteral("%1 × %2").arg(img.width()).arg(img.height());
            info["imageBitDepth"] = QStringLiteral("%1 bit").arg(img.depth());
        } else {
            info["imageResolution"] = QStringLiteral("");
            info["imageBitDepth"] = QStringLiteral("");
        }
    } else {
        info["imageResolution"] = QStringLiteral("");
        info["imageBitDepth"] = QStringLiteral("");
    }

    setCurrentFileInfo(info);
    setCurrentFilePath(entry.filePath);
    setCurrentModelIndex(index);
}

bool FileViewController::deleteFile(int index)
{
    if (index < 0 || index >= m_allEntries.size())
        return false;

    auto &entry = m_allEntries[index];
    QString filePath = entry.filePath;
    QString fileName = entry.fileName;

    if (!QFile::remove(filePath)) {
        QString msg = QStringLiteral("[删除失败] 无法删除文件: ") + filePath;
        m_logger->warn(msg);
        emit logMessage(msg);
        return false;
    }

    m_allEntries.removeAt(index);
    m_fileListModel->setFiles(m_allEntries);
    emit fileCountChanged();

    if (m_currentFilePath == filePath) {
        selectFile(m_allEntries.isEmpty() ? -1 : qMax(0, index - 1));
    } else if (m_currentModelIndex > index) {
        setCurrentModelIndex(m_currentModelIndex - 1);
    }

    m_logger->info(QStringLiteral("[删除] ") + fileName);
    return true;
}

void FileViewController::navigateToDir(const QString &path)
{
    if (!QFileInfo(path).isDir())
        return;

    if (m_workerRunning)
        cancel();

    m_thumbnailDelayTimer->stop();
    m_pendingThumbnailRequests.clear();
    setCurrentPath(path);

    m_allEntries.clear();
    m_fileListModel->clear();
    setCurrentFilePath(QString());
    setCurrentFileInfo(emptyFileInfo());

    emit logMessage(QStringLiteral("正在扫描文件夹..."));
    triggerScan();
}

void FileViewController::navigateUp()
{
    QDir dir(m_currentPath);
    if (!dir.cdUp())
        return;

    QString parent = dir.absolutePath();
    if (!parent.startsWith(m_sourceFolder))
        return;
    if (parent == m_currentPath)
        return;

    navigateToDir(parent);
}

int FileViewController::prevFileInCategory(int currentIndex) const
{
    if (currentIndex < 0 || currentIndex >= m_allEntries.size() || m_allEntries.size() <= 1)
        return -1;

    int category = m_allEntries.at(currentIndex).typeCategory;
    for (int i = currentIndex - 1; i >= 0; --i) {
        if (m_allEntries.at(i).typeCategory == category)
            return i;
    }
    for (int i = m_allEntries.size() - 1; i > currentIndex; --i) {
        if (m_allEntries.at(i).typeCategory == category)
            return i;
    }
    return -1;
}

int FileViewController::nextFileInCategory(int currentIndex) const
{
    if (currentIndex < 0 || currentIndex >= m_allEntries.size() || m_allEntries.size() <= 1)
        return -1;

    int category = m_allEntries.at(currentIndex).typeCategory;
    for (int i = currentIndex + 1; i < m_allEntries.size(); ++i) {
        if (m_allEntries.at(i).typeCategory == category)
            return i;
    }
    for (int i = 0; i < currentIndex; ++i) {
        if (m_allEntries.at(i).typeCategory == category)
            return i;
    }
    return -1;
}

void FileViewController::cancel()
{
    if (m_workerRunning) {
        m_workerThread.requestInterruption();
        m_workerThread.quit();
        if (!m_workerThread.wait(3000)) {
            m_workerThread.terminate();
            m_workerThread.wait();
        }
        m_workerRunning = false;
        emit isProcessingChanged();
    }
    if (m_thumbnailGenerator)
        m_thumbnailGenerator->cancel();
}

void FileViewController::startThumbnailGeneration()
{
    if (!m_thumbnailGenerator || m_pendingThumbnailRequests.isEmpty())
        return;

    m_thumbnailGenerator->cancel();
    m_thumbnailGenerator->requestThumbnails(m_pendingThumbnailRequests);
}

void FileViewController::onThumbnailReady(const QString &filePath, const QString &thumbnailPath)
{
    for (int i = 0; i < m_allEntries.size(); ++i) {
        if (m_allEntries[i].filePath == filePath
            && m_allEntries[i].typeCategory == 0
            && !m_allEntries[i].isDir) {
            m_allEntries[i].thumbnailPath = thumbnailPath;
            m_fileListModel->setThumbnailPath(i, thumbnailPath);
            break;
        }
    }
}

void FileViewController::reset()
{
    cancel();
    m_thumbnailDelayTimer->stop();
    m_pendingThumbnailRequests.clear();
    m_allEntries.clear();
    m_fileListModel->clear();
    setCurrentFilePath(QString());

    // 恢复上次设置
    m_sourceFolder = m_settings->sourceFolder();
    m_fileType = m_settings->fileType();
    m_recursive = m_settings->recursive();
    m_sortField = m_settings->sortField();
    m_sortAscending = m_settings->sortAscending();
    m_viewWay = m_settings->viewWay();
    m_viewMode = m_settings->viewMode();
    setCurrentPath(QString());

    setCurrentFileInfo(emptyFileInfo());

    emit sourceFolderChanged();
    emit fileTypeChanged();
    emit recursiveChanged();
    emit sortFieldChanged();
    emit sortAscendingChanged();
    emit viewWayChanged();
    emit viewModeChanged();
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
