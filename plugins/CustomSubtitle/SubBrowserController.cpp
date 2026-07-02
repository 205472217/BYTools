#include "SubBrowserController.h"
#include "CustomSubtitlePlugin.h"
#include "Logger.h"
#include "SettingsHelper.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QProcess>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>

static constexpr int kSearchTimeoutMs   = 30000;
static constexpr int kDownloadTimeoutMs = 120000;

// ══════════════════════════════════════════════════════════
SubBrowserController::SubBrowserController(PluginLogger *logger, QObject *parent)
    : QObject(parent), m_logger(logger)
{
    m_pythonPath  = findPython();
    m_scriptsDir  = findScriptsDir();
    m_pythonAvailable = !m_pythonPath.isEmpty() && !m_scriptsDir.isEmpty();

    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                 + "/CustomSubtitleCache";
    QDir().mkpath(m_cacheDir);

    if (m_pythonAvailable)
        checkDependencies();

    initSites();  // 扫描 python/<site>/search.py 文件夹

    QSettings &s = pluginGroupSettings(CustomSubtitlePlugin::PluginKey);
    s.sync();
    QString lastSite = s.value("browserLastSite").toString();
    if (!lastSite.isEmpty() && m_availableSites.contains(lastSite))
        m_currentSite = lastSite;
    else if (!m_availableSites.isEmpty())
        m_currentSite = m_availableSites.first();

    m_downloadPath = s.value("customSubtitleDownloadPath").toString();

    if (m_logger) {
        if (m_pythonAvailable)
            m_logger->info(QStringLiteral("SubBrowser: Python=%1, scripts=%2")
                               .arg(m_pythonPath, m_scriptsDir));
        else
            m_logger->warn(QStringLiteral("SubBrowser: Python 不可用，搜索功能将不可用"));
    }
}

SubBrowserController::~SubBrowserController()
{
    if (m_searchProcess) {
        m_searchProcess->disconnect();
        m_searchProcess->kill();
        m_searchProcess->waitForFinished(3000);
        m_searchProcess->deleteLater();
    }
    if (m_downloadProcess) {
        m_downloadProcess->disconnect();
        m_downloadProcess->kill();
        m_downloadProcess->waitForFinished(3000);
        m_downloadProcess->deleteLater();
    }
}

void SubBrowserController::initSites()
{
    m_availableSites.clear();
    if (m_scriptsDir.isEmpty()) return;

    QDir dir(m_scriptsDir);
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &name : entries) {
        if (QFile::exists(m_scriptsDir + "/" + name + "/search.py"))
            m_availableSites.append(name);
    }
}

// ── Python 探测 ──
QString SubBrowserController::findPython() const
{
    // 1) PATH 直接查找（适用于命令行启动场景）
    QString byPath = QStandardPaths::findExecutable("python3");
    if (byPath.isEmpty())
        byPath = QStandardPaths::findExecutable("python");
    if (!byPath.isEmpty()) {
        QProcess proc;
        proc.start(byPath, {"--version"});
        if (proc.waitForFinished(5000) && proc.exitCode() == 0)
            return byPath;
    }

#if defined(Q_OS_WIN)
    // 2) 遍历已知安装目录（Python.org / 微软商店安装）
    QString localAppData = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                           + "/../Local/Programs/Python";
    QStringList searchRoots = {
        localAppData,
        "C:/Python",
        "C:/Program Files/Python",
    };
    for (const QString &root : searchRoots) {
        QDir dir(root);
        if (!dir.exists()) continue;
        const QStringList entries = dir.entryList({"Python*"}, QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &sub : entries) {
            QString exe = root + "/" + sub + "/python.exe";
            QProcess proc;
            proc.start(exe, {"--version"});
            if (proc.waitForFinished(5000) && proc.exitCode() == 0)
                return exe;
        }
    }

    // 3) 兜底：直接尝试 Python 3.10~3.13 的常见路径
    QStringList fallbacks = {
        localAppData + "/Python313/python.exe",
        localAppData + "/Python312/python.exe",
        localAppData + "/Python311/python.exe",
        localAppData + "/Python310/python.exe",
        "C:/Python313/python.exe",
        "C:/Python312/python.exe",
        "C:/Python311/python.exe",
        "C:/Python310/python.exe",
        "C:/Python/Python313/python.exe",
        "C:/Python/Python312/python.exe",
        "C:/Python/Python311/python.exe",
        "C:/Python/Python310/python.exe",
    };
    for (const QString &exe : fallbacks) {
        if (!QFile::exists(exe)) continue;
        QProcess proc;
        proc.start(exe, {"--version"});
        if (proc.waitForFinished(5000) && proc.exitCode() == 0)
            return exe;
    }
#endif

    return {};
}

QString SubBrowserController::findScriptsDir() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/plugins/customsubtitle/python",
        appDir + "/../plugins/customsubtitle/python",
        appDir + "/../../plugins/customsubtitle/python",
    };
    for (const QString &path : candidates)
        if (QDir(path).exists()) return path;
    return {};
}

bool SubBrowserController::ensurePythonAvailable()
{
    if (m_pythonAvailable) return true;
    m_pythonPath = findPython();
    m_scriptsDir = findScriptsDir();
    m_pythonAvailable = !m_pythonPath.isEmpty() && !m_scriptsDir.isEmpty();
    emit pythonAvailableChanged();
    return m_pythonAvailable;
}

void SubBrowserController::checkDependencies()
{
    if (!m_pythonAvailable) {
        m_dependenciesMet = false;
        emit dependenciesMetChanged();
        return;
    }

    QProcess proc;
    proc.start(m_pythonPath, {"-c", "import lxml"});
    m_dependenciesMet = proc.waitForFinished(10000) && proc.exitCode() == 0;
    emit dependenciesMetChanged();
}

// ══════════════════════════════════════════════════════════
//  Getters / Setters
// ══════════════════════════════════════════════════════════
QStringList SubBrowserController::availableSites() const { return m_availableSites; }
QString SubBrowserController::currentSite()   const { return m_currentSite; }
QVariantList SubBrowserController::languageFilterOptions() const
{
    // 语言筛选选项：显示名 → QML 下拉，code → 后端标识，pageLabel → 详情页匹配文本
    // 定义只此一处，QML 和 Python 都动态获取
    QVariantList opts;
    auto add = [&](const char *display, const char *code, const char *pageLabel) {
        QVariantMap m;
        m["display"]    = QString::fromUtf8(display);
        m["code"]       = QString::fromUtf8(code);
        m["pageLabel"]  = QString::fromUtf8(pageLabel);
        opts.append(m);
    };
    add("中文简体", "zh-CN", "Chinese (Simplified)");
    add("中文繁体", "zh-TW", "Chinese (Traditional)");
    add("英文",     "en",    "English");
    return opts;
}

QString SubBrowserController::keyword()        const { return m_keyword; }
QString SubBrowserController::languageFilter() const { return m_languageFilter; }
bool SubBrowserController::searching()        const { return m_searching; }
QVariantList SubBrowserController::searchResults() const { return m_searchResults; }
QString SubBrowserController::searchStatus()  const { return m_searchStatus; }
QString SubBrowserController::downloadPath()  const { return m_downloadPath; }
bool SubBrowserController::downloading()      const { return m_downloading; }
QString SubBrowserController::downloadingFile() const { return m_downloadingFile; }
bool SubBrowserController::pythonAvailable()  const { return m_pythonAvailable; }
bool SubBrowserController::dependenciesMet() const { return m_dependenciesMet; }
int SubBrowserController::searchProgress() const { return m_searchProgress; }
QString SubBrowserController::searchProgressMessage() const { return m_searchProgressMessage; }
bool SubBrowserController::previewing() const { return m_previewing; }
QString SubBrowserController::previewContent() const { return m_previewContent; }
int SubBrowserController::cachedIndex() const { return m_cachedIndex; }

void SubBrowserController::setCurrentSite(const QString &site)
{
    if (m_currentSite != site) {
        m_currentSite = site;
        emit currentSiteChanged();
        QSettings &s = pluginGroupSettings(CustomSubtitlePlugin::PluginKey);
        s.setValue("browserLastSite", site);
    }
}

void SubBrowserController::setKeyword(const QString &keyword)
{
    if (m_keyword != keyword) { m_keyword = keyword; emit keywordChanged(); }
}

void SubBrowserController::setLanguageFilter(const QString &filter)
{
    if (m_languageFilter != filter) { m_languageFilter = filter; emit languageFilterChanged(); }
}

void SubBrowserController::setDownloadPath(const QString &path)
{
    if (m_downloadPath != path) { m_downloadPath = path; emit downloadPathChanged(); }
}

// ══════════════════════════════════════════════════════════
//  搜索
// ══════════════════════════════════════════════════════════
void SubBrowserController::search()
{
    if (m_searching) return;

    if (!ensurePythonAvailable()) {
        m_searchStatus = QStringLiteral("Python 不可用，请安装 Python 3.10+");
        emit searchStatusChanged();
        emit logMessage(m_searchStatus);
        return;
    }

    if (!m_dependenciesMet) {
        checkDependencies();
        if (!m_dependenciesMet) {
            m_searchStatus = QStringLiteral(
                "缺少 Python 依赖 lxml 库，请运行:\n"
                "pip install lxml");
            emit searchStatusChanged();
            emit logMessage(m_searchStatus);
            return;
        }
    }

    if (m_keyword.trimmed().isEmpty()) {
        m_searchStatus = QStringLiteral("请输入搜索关键字");
        emit searchStatusChanged();
        return;
    }

    // m_currentSite 即 python/ 下的文件夹名，直接传给 Python
    const QString &siteKey = m_currentSite;

    // 新搜索前清空预览缓存和下载状态
    m_downloadedIndices.clear();
    clearPreview();
    QDir cacheDir(m_cacheDir);
    if (cacheDir.exists())
        cacheDir.removeRecursively();
    QDir().mkpath(m_cacheDir);

    m_searching = true;
    m_searchResults.clear();
    m_searchStatus = QStringLiteral("正在搜索...");
    emit searchingChanged();
    emit searchResultsChanged();
    emit searchStatusChanged();

    if (m_logger)
        m_logger->info(QStringLiteral("SubBrowser: 搜索 '%1' @ %2").arg(m_keyword, siteKey));

    QJsonObject req;
    req["site"] = siteKey;
    req["keyword"] = m_keyword.trimmed();
    // m_languageFilter 存的是 code（如 zh-CN），查出 pageLabel（如 "Chinese (Simplified)"）传给 Python
    QString pageLabel;
    if (!m_languageFilter.isEmpty()) {
        for (const auto &opt : languageFilterOptions()) {
            QVariantMap m = opt.toMap();
            if (m.value("code").toString() == m_languageFilter) {
                pageLabel = m.value("pageLabel").toString();
                break;
            }
        }
    }
    if (!pageLabel.isEmpty())
        req["language_filter"] = pageLabel;

    static constexpr int kDefaultMaxResults = 50;
    static constexpr int kMaxResultsMin    = 5;
    static constexpr int kMaxResultsMax    = 100;

    QSettings &s = pluginGroupSettings(CustomSubtitlePlugin::PluginKey);
    int maxResults = s.value("browserMaxResults", kDefaultMaxResults).toInt();
    if (maxResults < kMaxResultsMin || maxResults > kMaxResultsMax)
        maxResults = kDefaultMaxResults;
    req["max_results"] = maxResults;

    QByteArray jsonData = QJsonDocument(req).toJson(QJsonDocument::Compact);

    if (m_searchProcess) {
        m_searchProcess->disconnect();
        m_searchProcess->kill();
        m_searchProcess->waitForFinished(3000);
        m_searchProcess->deleteLater();
    }

    m_searchProgress = 0;
    m_searchProgressMessage.clear();
    m_searchBuffer.clear();
    emit searchProgressChanged();
    emit searchProgressMessageChanged();

    m_searchProcess = new QProcess(this);
    connect(m_searchProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SubBrowserController::onSearchProcessFinished);
    connect(m_searchProcess, &QProcess::errorOccurred,
            this, &SubBrowserController::onSearchProcessError);
    connect(m_searchProcess, &QProcess::readyReadStandardOutput,
            this, &SubBrowserController::onSearchStdoutReady);

    if (!m_searchTimer) {
        m_searchTimer = new QTimer(this);
        m_searchTimer->setSingleShot(true);
        connect(m_searchTimer, &QTimer::timeout, this, &SubBrowserController::onSearchTimeout);
    }

    QString script = m_scriptsDir + "/search.py";
    m_searchProcess->start(m_pythonPath, {"-u", script});
    m_searchProcess->write(jsonData);
    m_searchProcess->closeWriteChannel();
    m_searchTimer->start(kSearchTimeoutMs);
}

void SubBrowserController::onSearchStdoutReady()
{
    m_searchBuffer.append(m_searchProcess->readAllStandardOutput());

    while (true) {
        int idx = m_searchBuffer.indexOf('\n');
        if (idx < 0)
            break;

        QByteArray line = m_searchBuffer.left(idx).trimmed();
        m_searchBuffer.remove(0, idx + 1);

        if (line.isEmpty())
            continue;

        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(line, &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject())
            continue;

        QJsonObject obj = doc.object();

        if (obj.contains("progress")) {
            int current = obj.value("progress").toInt();
            int total = obj.value("total").toInt();
            m_searchProgress = total > 0 ? current * 100 / total : 0;
            m_searchProgressMessage = obj.value("message").toString();
            emit searchProgressChanged();
            emit searchProgressMessageChanged();
            if (m_logger)
                m_logger->info(QStringLiteral("SubBrowser: 进度 %1/%2").arg(current).arg(total));
            m_searchTimer->start(kSearchTimeoutMs);
            continue;
        }

        if (obj.contains("ok") || obj.contains("error")) {
            m_searchBuffer.prepend(line + '\n');
            break;
        }
    }
}

void SubBrowserController::onSearchProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_searchTimer->stop();

    if (!m_searchProcess)
        return;

    // Read any remaining stdout not yet consumed by onSearchStdoutReady
    m_searchBuffer.append(m_searchProcess->readAllStandardOutput());
    QByteArray err = m_searchProcess->readAllStandardError();

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QString msg = QString::fromUtf8(err).trimmed();
        if (msg.isEmpty())
            msg = QString::fromUtf8(m_searchBuffer).trimmed();
        m_searchStatus = QStringLiteral("搜索失败 (exit=%1): %2")
                             .arg(exitCode).arg(msg);
        m_searching = false;
        m_searchBuffer.clear();
        emit searchingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
        if (m_logger) m_logger->warn(m_searchStatus);
        m_searchProcess->deleteLater(); m_searchProcess = nullptr;
        return;
    }

    m_searchProcess->deleteLater(); m_searchProcess = nullptr;

    if (!err.trimmed().isEmpty()) {
        auto lines = QString::fromUtf8(err).trimmed().split('\n');
        for (const auto &line : lines) {
            if (m_logger) m_logger->info(QStringLiteral("[Python] %1").arg(line.trimmed()));
        }
    }

    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(m_searchBuffer, &pe);
    m_searchBuffer.clear();
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        m_searchStatus = QStringLiteral("搜索结果解析失败: %1").arg(pe.errorString());
        m_searching = false;
        emit searchingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
        return;
    }

    QJsonObject root = doc.object();
    if (!root.value("ok").toBool(false)) {
        m_searchStatus = root.value("error").toString(QStringLiteral("搜索失败"));
        m_searching = false;
        emit searchingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
        return;
    }

    QVariantList results;
    for (const QJsonValue &v : root.value("results").toArray()) {
        QJsonObject item = v.toObject();
        QVariantMap map;
        map["site"]        = item.value("site").toString();
        map["language"]    = item.value("language").toString();
        map["fileName"]    = item.value("file_name").toString();
        map["downloadUrl"] = item.value("download_url").toString();
        results.append(map);
    }

    m_searchResults = results;
    if (results.isEmpty())
        m_searchStatus = QStringLiteral("搜索完成，未找到相关内容");
    else
        m_searchStatus = QStringLiteral("找到 %1 条字幕").arg(results.size());
    m_searching = false;
    emit searchResultsChanged(); emit searchStatusChanged(); emit searchingChanged();
    emit logMessage(m_searchStatus);
    if (m_logger) m_logger->info(m_searchStatus);
}

void SubBrowserController::onSearchProcessError(QProcess::ProcessError error)
{
    m_searchTimer->stop();

    if (!m_searchProcess)
        return;

    QString msg = (error == QProcess::FailedToStart)
        ? QStringLiteral("Python 进程启动失败，请确认已安装 Python")
        : QStringLiteral("Python 进程错误");
    m_searchStatus = msg;
    m_searching = false;
    m_searchBuffer.clear();
    emit searchingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
    if (m_logger) m_logger->error("SubBrowser: " + msg);
    m_searchProcess->deleteLater(); m_searchProcess = nullptr;
}

void SubBrowserController::stopSearch()
{
    if (m_searchProcess && m_searchProcess->state() != QProcess::NotRunning) {
        m_searchProcess->disconnect();
        m_searchProcess->kill();
        m_searchProcess->waitForFinished(3000);
        m_searchProcess->deleteLater();
        m_searchProcess = nullptr;
        m_searchTimer->stop();
    }
    m_searchBuffer.clear();
    m_searching = false;
    m_searchStatus = QStringLiteral("已停止搜索");
    emit searchingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
    if (m_logger) m_logger->info("SubBrowser: 搜索已手动停止");
}

void SubBrowserController::onSearchTimeout()
{
    if (m_searchProcess && m_searchProcess->state() != QProcess::NotRunning) {
        m_searchProcess->disconnect();
        m_searchProcess->kill();
        m_searchProcess->waitForFinished(3000);
        m_searchProcess->deleteLater();
        m_searchProcess = nullptr;
    }
    m_searchBuffer.clear();
    m_searching = false;
    m_searchStatus = QStringLiteral("搜索超时，请检查网络连接后重试");
    emit searchingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
    if (m_logger) m_logger->warn("SubBrowser: 搜索超时");
}

// ══════════════════════════════════════════════════════════
//  预览（下载到缓存）
// ══════════════════════════════════════════════════════════
void SubBrowserController::preview(int index)
{
    if (m_previewing || m_downloading) return;
    if (index < 0 || index >= m_searchResults.size()) return;

    QVariantMap item = m_searchResults[index].toMap();
    QString url      = item.value("downloadUrl").toString();
    QString fileName = item.value("fileName").toString();
    QString language = item.value("language").toString();
    if (url.isEmpty()) return;

    m_cachedIndex = index;
    m_cachedFilePath.clear();
    m_previewContent.clear();
    m_previewing = true;
    m_isPreviewDownload = true;
    emit cachedIndexChanged();
    emit previewContentChanged();
    emit previewingChanged();

    if (m_logger)
        m_logger->info(QStringLiteral("SubBrowser: 预览下载 '%1' (%2) → %3")
                           .arg(fileName, language, m_cacheDir));

    QJsonObject req;
    req["url"]        = url;
    req["file_name"]  = fileName;
    req["output_dir"] = m_cacheDir;
    req["language"]   = language;
    QByteArray jsonData = QJsonDocument(req).toJson(QJsonDocument::Compact);

    if (m_downloadProcess) {
        m_downloadProcess->disconnect();
        m_downloadProcess->kill();
        m_downloadProcess->waitForFinished(3000);
        m_downloadProcess->deleteLater();
    }

    m_downloadProcess = new QProcess(this);
    connect(m_downloadProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SubBrowserController::onDownloadProcessFinished);
    connect(m_downloadProcess, &QProcess::errorOccurred,
            this, &SubBrowserController::onDownloadProcessError);

    if (!m_downloadTimer) {
        m_downloadTimer = new QTimer(this);
        m_downloadTimer->setSingleShot(true);
        connect(m_downloadTimer, &QTimer::timeout, this, &SubBrowserController::onDownloadTimeout);
    }

    QString script = m_scriptsDir + "/download.py";
    m_downloadProcess->start(m_pythonPath, {"-u", script});
    m_downloadProcess->write(jsonData);
    m_downloadProcess->closeWriteChannel();
    m_downloadTimer->start(kDownloadTimeoutMs);
}

void SubBrowserController::clearPreview()
{
    m_cachedIndex = -1;
    m_cachedFilePath.clear();
    m_previewContent.clear();
    emit cachedIndexChanged();
    emit previewContentChanged();
}

bool SubBrowserController::isDownloaded(int index) const
{
    return m_downloadedIndices.contains(index);
}

void SubBrowserController::savePreviewToDownload()
{
    if (m_downloadPath.isEmpty()) {
        m_searchStatus = QStringLiteral("请先在上方设置字幕下载路径");
        emit searchStatusChanged(); emit logMessage(m_searchStatus);
        return;
    }
    if (m_cachedIndex < 0 || m_cachedFilePath.isEmpty()) {
        m_searchStatus = QStringLiteral("没有可保存的预览缓存");
        emit searchStatusChanged(); emit logMessage(m_searchStatus);
        return;
    }

    QFileInfo fi(m_cachedFilePath);
    QString destPath = m_downloadPath + "/" + fi.fileName();
    if (QFile::exists(destPath))
        QFile::remove(destPath);
    if (QFile::copy(m_cachedFilePath, destPath)) {
        m_downloadedIndices.insert(m_cachedIndex);
        m_searchStatus = QStringLiteral("已保存: %1").arg(destPath);
        if (m_logger) m_logger->info(m_searchStatus);
    } else {
        m_searchStatus = QStringLiteral("文件保存失败: %1").arg(destPath);
        if (m_logger) m_logger->warn(m_searchStatus);
    }
    emit searchStatusChanged();
    emit logMessage(m_searchStatus);
}

// ══════════════════════════════════════════════════════════
//  下载
// ══════════════════════════════════════════════════════════
void SubBrowserController::download(int index)
{
    if (m_downloading) return;
    if (index < 0 || index >= m_searchResults.size()) return;

    if (m_downloadPath.isEmpty()) {
        m_searchStatus = QStringLiteral("请先在上方设置字幕下载路径");
        emit searchStatusChanged(); emit logMessage(m_searchStatus);
        return;
    }

    // 如果有预览缓存且匹配，直接从缓存复制到下载目录
    if (index == m_cachedIndex && !m_cachedFilePath.isEmpty()) {
        QFileInfo fi(m_cachedFilePath);
        QString destPath = m_downloadPath + "/" + fi.fileName();
        if (QFile::exists(destPath))
            QFile::remove(destPath);
        if (QFile::copy(m_cachedFilePath, destPath)) {
            m_downloadedIndices.insert(index);
            m_searchStatus = QStringLiteral("下载完成: %1").arg(destPath);
            if (m_logger) m_logger->info(m_searchStatus);
        } else {
            m_searchStatus = QStringLiteral("文件复制失败: %1").arg(destPath);
            if (m_logger) m_logger->warn(m_searchStatus);
        }
        emit searchStatusChanged();
        emit logMessage(m_searchStatus);
        return;
    }

    QVariantMap item = m_searchResults[index].toMap();
    QString url      = item.value("downloadUrl").toString();
    QString fileName = item.value("fileName").toString();
    QString language = item.value("language").toString();
    if (url.isEmpty()) return;

    m_currentDownloadIndex = index;
    m_downloading = true;
    m_downloadingFile = fileName;
    emit downloadingChanged(); emit downloadingFileChanged();

    if (m_logger)
        m_logger->info(QStringLiteral("SubBrowser: 下载 '%1' (%2)\n  url=%3")
                           .arg(fileName, language, url));

    QJsonObject req;
    req["url"]        = url;
    req["file_name"]  = fileName;
    req["output_dir"] = m_downloadPath;
    req["language"]   = language;
    QByteArray jsonData = QJsonDocument(req).toJson(QJsonDocument::Compact);

    if (m_downloadProcess) {
        m_downloadProcess->disconnect();
        m_downloadProcess->kill();
        m_downloadProcess->waitForFinished(3000);
        m_downloadProcess->deleteLater();
    }

    m_downloadProcess = new QProcess(this);
    connect(m_downloadProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SubBrowserController::onDownloadProcessFinished);
    connect(m_downloadProcess, &QProcess::errorOccurred,
            this, &SubBrowserController::onDownloadProcessError);

    if (!m_downloadTimer) {
        m_downloadTimer = new QTimer(this);
        m_downloadTimer->setSingleShot(true);
        connect(m_downloadTimer, &QTimer::timeout, this, &SubBrowserController::onDownloadTimeout);
    }

    QString script = m_scriptsDir + "/download.py";
    m_downloadProcess->start(m_pythonPath, {"-u", script});
    m_downloadProcess->write(jsonData);
    m_downloadProcess->closeWriteChannel();
    m_downloadTimer->start(kDownloadTimeoutMs);
}

void SubBrowserController::onDownloadProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_downloadTimer->stop();

    if (!m_downloadProcess)
        return;

    bool wasPreview = m_isPreviewDownload;
    m_isPreviewDownload = false;

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QByteArray err = m_downloadProcess->readAllStandardError();
        QString msg = QStringLiteral("下载失败: %1").arg(QString::fromUtf8(err).trimmed());

        if (wasPreview) {
            m_previewing = false;
            m_cachedIndex = -1;
            emit previewingChanged();
            emit cachedIndexChanged();
        } else {
            m_currentDownloadIndex = -1;
            m_downloading = false;
            emit downloadingChanged();
        }

        m_searchStatus = msg;
        emit searchStatusChanged(); emit logMessage(m_searchStatus);
        if (m_logger) m_logger->warn(m_searchStatus);
        m_downloadProcess->deleteLater(); m_downloadProcess = nullptr;
        return;
    }

    QByteArray out = m_downloadProcess->readAllStandardOutput();
    m_downloadProcess->deleteLater(); m_downloadProcess = nullptr;

    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(out, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        QString msg = QStringLiteral("下载结果解析失败");

        if (wasPreview) {
            m_previewing = false;
            m_cachedIndex = -1;
            emit previewingChanged();
            emit cachedIndexChanged();
        } else {
            m_currentDownloadIndex = -1;
            m_downloading = false;
            emit downloadingChanged();
        }

        m_searchStatus = msg;
        emit searchStatusChanged(); emit logMessage(m_searchStatus);
        return;
    }

    QJsonObject root = doc.object();

    if (wasPreview) {
        m_previewing = false;
        emit previewingChanged();

        if (root.value("ok").toBool(false)) {
            m_cachedFilePath = root.value("file_path").toString();
            QFile file(m_cachedFilePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                m_previewContent = QString::fromUtf8(file.readAll());
                file.close();
            }
            emit cachedIndexChanged();
            emit previewContentChanged();
            m_searchStatus = QStringLiteral("预览就绪");
            if (m_logger) m_logger->info(QStringLiteral("SubBrowser: 预览就绪 %1").arg(m_cachedFilePath));
        } else {
            m_cachedIndex = -1;
            emit cachedIndexChanged();
            m_searchStatus = QStringLiteral("预览下载失败: %1").arg(root.value("error").toString());
            if (m_logger) m_logger->warn(m_searchStatus);
        }
    } else {
        if (root.value("ok").toBool(false)) {
            m_downloadedIndices.insert(m_currentDownloadIndex);
            m_searchStatus = QStringLiteral("下载完成: %1").arg(root.value("file_path").toString());
            if (m_logger) m_logger->info(m_searchStatus);
        } else {
            m_currentDownloadIndex = -1;
            m_searchStatus = QStringLiteral("下载失败: %1").arg(root.value("error").toString());
            if (m_logger) m_logger->warn(m_searchStatus);
        }
        m_downloading = false;
        emit downloadingChanged();
    }

    emit searchStatusChanged(); emit logMessage(m_searchStatus);
}

void SubBrowserController::onDownloadProcessError(QProcess::ProcessError error)
{
    m_downloadTimer->stop();

    if (!m_downloadProcess)
        return;

    bool wasPreview = m_isPreviewDownload;
    m_isPreviewDownload = false;

    QString msg = (error == QProcess::FailedToStart)
        ? QStringLiteral("Python 进程启动失败")
        : QStringLiteral("下载进程错误");

    if (wasPreview) {
        m_previewing = false;
        m_cachedIndex = -1;
        emit previewingChanged();
        emit cachedIndexChanged();
    } else {
        m_currentDownloadIndex = -1;
        m_downloading = false;
        emit downloadingChanged();
    }

    m_searchStatus = msg;
    emit searchStatusChanged(); emit logMessage(m_searchStatus);
    m_downloadProcess->deleteLater(); m_downloadProcess = nullptr;
}

void SubBrowserController::onDownloadTimeout()
{
    if (m_downloadProcess && m_downloadProcess->state() != QProcess::NotRunning) {
        m_downloadProcess->disconnect();
        m_downloadProcess->kill();
        m_downloadProcess->waitForFinished(3000);
        m_downloadProcess->deleteLater();
        m_downloadProcess = nullptr;
    }

    bool wasPreview = m_isPreviewDownload;
    m_isPreviewDownload = false;

    if (wasPreview) {
        m_previewing = false;
        m_cachedIndex = -1;
        emit previewingChanged();
        emit cachedIndexChanged();
    } else {
        m_currentDownloadIndex = -1;
        m_downloading = false;
        emit downloadingChanged();
    }

    m_searchStatus = QStringLiteral("下载超时");
    emit searchStatusChanged(); emit logMessage(m_searchStatus);
}

void SubBrowserController::openDownloadFolder()
{
    if (!m_downloadPath.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_downloadPath));
}