#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QProcess>
#include <QTimer>

class PluginLogger;

/// 字幕网站浏览器控制器
/// 通过 QProcess 调用 Python 脚本完成字幕搜索与下载，C++ 侧不强制依赖 Python
class SubBrowserController : public QObject
{
    Q_OBJECT

    // === 站点 ===
    Q_PROPERTY(QStringList availableSites READ availableSites NOTIFY availableSitesChanged)
    Q_PROPERTY(QString currentSite READ currentSite WRITE setCurrentSite NOTIFY currentSiteChanged)

    // === 语言筛选选项（显示名 + 后端 code + 页面匹配文本，只此一处定义） ===
    Q_PROPERTY(QVariantList languageFilterOptions READ languageFilterOptions CONSTANT)

    // === 搜索 ===
    Q_PROPERTY(QString keyword READ keyword WRITE setKeyword NOTIFY keywordChanged)
    Q_PROPERTY(QString languageFilter READ languageFilter WRITE setLanguageFilter NOTIFY languageFilterChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QString searchStatus READ searchStatus NOTIFY searchStatusChanged)

    // === 下载 ===
    Q_PROPERTY(QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY downloadPathChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)
    Q_PROPERTY(QString downloadingFile READ downloadingFile NOTIFY downloadingFileChanged)
    Q_PROPERTY(bool pythonAvailable READ pythonAvailable NOTIFY pythonAvailableChanged)
    Q_PROPERTY(bool dependenciesMet READ dependenciesMet NOTIFY dependenciesMetChanged)

public:
    explicit SubBrowserController(PluginLogger *logger, QObject *parent = nullptr);
    ~SubBrowserController();

    QStringList availableSites() const;
    QString currentSite() const;
    QVariantList languageFilterOptions() const;
    QString keyword() const;
    QString languageFilter() const;
    bool searching() const;
    QVariantList searchResults() const;
    QString searchStatus() const;
    QString downloadPath() const;
    bool downloading() const;
    QString downloadingFile() const;
    bool pythonAvailable() const;
    bool dependenciesMet() const;

    void setCurrentSite(const QString &site);
    void setKeyword(const QString &keyword);
    void setLanguageFilter(const QString &filter);
    void setDownloadPath(const QString &path);

    Q_INVOKABLE void search();
    Q_INVOKABLE void stopSearch();
    Q_INVOKABLE void download(int index);
    Q_INVOKABLE void openDownloadFolder();
    Q_INVOKABLE void checkDependencies();

signals:
    void availableSitesChanged();
    void currentSiteChanged();
    void keywordChanged();
    void languageFilterChanged();
    void searchingChanged();
    void searchResultsChanged();
    void searchStatusChanged();
    void downloadPathChanged();
    void downloadingChanged();
    void downloadingFileChanged();
    void pythonAvailableChanged();
    void dependenciesMetChanged();
    void logMessage(const QString &message);

private slots:
    void onSearchProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSearchProcessError(QProcess::ProcessError error);
    void onDownloadProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDownloadProcessError(QProcess::ProcessError error);
    void onSearchTimeout();
    void onDownloadTimeout();

private:
    void initSites();
    QString findPython() const;
    QString findScriptsDir() const;
    bool ensurePythonAvailable();

    PluginLogger *m_logger;

    QStringList m_availableSites;
    QString m_currentSite;

    QString m_keyword;
    QString m_languageFilter;
    bool m_searching = false;
    QVariantList m_searchResults;
    QString m_searchStatus;

    QString m_downloadPath;
    bool m_downloading = false;
    QString m_downloadingFile;

    bool m_pythonAvailable = false;
    bool m_dependenciesMet = false;
    QString m_pythonPath;
    QString m_scriptsDir;

    QProcess *m_searchProcess = nullptr;
    QProcess *m_downloadProcess = nullptr;
    QTimer *m_searchTimer = nullptr;
    QTimer *m_downloadTimer = nullptr;
};