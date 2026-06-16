#include "SubBrowserController.h"
#include "Logger.h"
#include "SettingsHelper.h"

#include <QCoreApplication>
#include <QDir>
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

    initSites();  // 扫描 python/<site>/search.py 文件夹

    QSettings &s = pluginGroupSettings("custom-subtitle");
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
        m_searchProcess->kill();
        m_searchProcess->waitForFinished(3000);
        m_searchProcess->deleteLater();
    }
    if (m_downloadProcess) {
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
#if defined(Q_OS_WIN)
    QStringList candidates = {
        "python", "python3",
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
            + "/../Local/Programs/Python/Python312/python.exe",
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
            + "/../Local/Programs/Python/Python311/python.exe",
        "C:/Python312/python.exe", "C:/Python311/python.exe",
    };
#else
    QStringList candidates = {"python3", "python"};
#endif
    for (const QString &c : candidates) {
        QProcess proc;
        proc.start(c, {"--version"});
        if (proc.waitForFinished(5000) && proc.exitCode() == 0) return c;
    }
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

// ══════════════════════════════════════════════════════════
//  Getters / Setters
// ══════════════════════════════════════════════════════════
QStringList SubBrowserController::availableSites() const { return m_availableSites; }
QString SubBrowserController::currentSite()   const { return m_currentSite; }
QString SubBrowserController::keyword()        const { return m_keyword; }
QString SubBrowserController::languageFilter() const { return m_languageFilter; }
bool SubBrowserController::searching()        const { return m_searching; }
QVariantList SubBrowserController::searchResults() const { return m_searchResults; }
QString SubBrowserController::searchStatus()  const { return m_searchStatus; }
QString SubBrowserController::downloadPath()  const { return m_downloadPath; }
bool SubBrowserController::downloading()      const { return m_downloading; }
QString SubBrowserController::downloadingFile() const { return m_downloadingFile; }
bool SubBrowserController::pythonAvailable()  const { return m_pythonAvailable; }

void SubBrowserController::setCurrentSite(const QString &site)
{
    if (m_currentSite != site) {
        m_currentSite = site;
        emit currentSiteChanged();
        QSettings &s = pluginGroupSettings("custom-subtitle");
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
        m_searchStatus = QStringLiteral(
            "Python 不可用，请安装 Python 3.10+ 并安装 scrapling 库\n"
            "安装命令: pip install scrapling");
        emit searchStatusChanged();
        emit logMessage(m_searchStatus);
        return;
    }

    if (m_keyword.trimmed().isEmpty()) {
        m_searchStatus = QStringLiteral("请输入搜索关键字");
        emit searchStatusChanged();
        return;
    }

    // m_currentSite 即 python/ 下的文件夹名，直接传给 Python
    const QString &siteKey = m_currentSite;

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
    if (!m_languageFilter.isEmpty() && m_languageFilter != "全部")
        req["language_filter"] = m_languageFilter;
    QByteArray jsonData = QJsonDocument(req).toJson(QJsonDocument::Compact);

    if (m_searchProcess) {
        m_searchProcess->kill();
        m_searchProcess->waitForFinished(3000);
        m_searchProcess->deleteLater();
    }

    m_searchProcess = new QProcess(this);
    connect(m_searchProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SubBrowserController::onSearchProcessFinished);
    connect(m_searchProcess, &QProcess::errorOccurred,
            this, &SubBrowserController::onSearchProcessError);

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

void SubBrowserController::onSearchProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_searchTimer->stop();

    QByteArray out = m_searchProcess->readAllStandardOutput();
    QByteArray err = m_searchProcess->readAllStandardError();

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QString msg = QString::fromUtf8(err).trimmed();
        if (msg.isEmpty())
            msg = QString::fromUtf8(out).trimmed();
        m_searchStatus = QStringLiteral("搜索失败 (exit=%1): %2")
                             .arg(exitCode).arg(msg);
        m_searching = false;
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
    QJsonDocument doc = QJsonDocument::fromJson(out, &pe);
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
    m_searchStatus = QStringLiteral("找到 %1 条字幕").arg(results.size());
    m_searching = false;
    emit searchResultsChanged(); emit searchStatusChanged(); emit searchingChanged();
    emit logMessage(m_searchStatus);
    if (m_logger) m_logger->info(m_searchStatus);
}

void SubBrowserController::onSearchProcessError(QProcess::ProcessError error)
{
    m_searchTimer->stop();
    QString msg = (error == QProcess::FailedToStart)
        ? QStringLiteral("Python 进程启动失败，请确认已安装 Python")
        : QStringLiteral("Python 进程错误");
    m_searchStatus = msg;
    m_searching = false;
    emit searchingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
    if (m_logger) m_logger->error("SubBrowser: " + msg);
    if (m_searchProcess) { m_searchProcess->deleteLater(); m_searchProcess = nullptr; }
}

void SubBrowserController::onSearchTimeout()
{
    if (m_searchProcess && m_searchProcess->state() != QProcess::NotRunning) {
        m_searchProcess->kill();
        m_searchStatus = QStringLiteral("搜索超时，请检查网络连接后重试");
        m_searching = false;
        emit searchingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
    }
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

    QVariantMap item = m_searchResults[index].toMap();
    QString url      = item.value("downloadUrl").toString();
    QString fileName = item.value("fileName").toString();
    QString language = item.value("language").toString();
    if (url.isEmpty()) return;

    m_downloading = true;
    m_downloadingFile = fileName;
    emit downloadingChanged(); emit downloadingFileChanged();

    if (m_logger)
        m_logger->info(QStringLiteral("SubBrowser: 下载 '%1' (%2)").arg(fileName, language));

    QJsonObject req;
    req["url"]        = url;
    req["file_name"]  = fileName;
    req["output_dir"] = m_downloadPath;
    req["language"]   = language;
    QByteArray jsonData = QJsonDocument(req).toJson(QJsonDocument::Compact);

    if (m_downloadProcess) {
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

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QByteArray err = m_downloadProcess->readAllStandardError();
        m_searchStatus = QStringLiteral("下载失败: %1").arg(QString::fromUtf8(err).trimmed());
        m_downloading = false;
        emit downloadingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
        if (m_logger) m_logger->warn(m_searchStatus);
        m_downloadProcess->deleteLater(); m_downloadProcess = nullptr;
        return;
    }

    QByteArray out = m_downloadProcess->readAllStandardOutput();
    m_downloadProcess->deleteLater(); m_downloadProcess = nullptr;

    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(out, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        m_searchStatus = QStringLiteral("下载结果解析失败");
        m_downloading = false;
        emit downloadingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
        return;
    }

    QJsonObject root = doc.object();
    if (root.value("ok").toBool(false)) {
        m_searchStatus = QStringLiteral("下载完成: %1").arg(root.value("file_path").toString());
        if (m_logger) m_logger->info(m_searchStatus);
    } else {
        m_searchStatus = QStringLiteral("下载失败: %1").arg(root.value("error").toString());
        if (m_logger) m_logger->warn(m_searchStatus);
    }

    m_downloading = false;
    emit downloadingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
}

void SubBrowserController::onDownloadProcessError(QProcess::ProcessError error)
{
    m_downloadTimer->stop();
    QString msg = (error == QProcess::FailedToStart)
        ? QStringLiteral("Python 进程启动失败")
        : QStringLiteral("下载进程错误");
    m_searchStatus = msg;
    m_downloading = false;
    emit downloadingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
    if (m_downloadProcess) { m_downloadProcess->deleteLater(); m_downloadProcess = nullptr; }
}

void SubBrowserController::onDownloadTimeout()
{
    if (m_downloadProcess && m_downloadProcess->state() != QProcess::NotRunning) {
        m_downloadProcess->kill();
        m_searchStatus = QStringLiteral("下载超时");
        m_downloading = false;
        emit downloadingChanged(); emit searchStatusChanged(); emit logMessage(m_searchStatus);
    }
}

void SubBrowserController::openDownloadFolder()
{
    if (!m_downloadPath.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_downloadPath));
}