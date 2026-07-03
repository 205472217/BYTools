#include "GlobalConfig.h"
#include "SettingsHelper.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QProcess>

GlobalConfig *GlobalConfig::instance()
{
    static GlobalConfig config;
    return &config;
}

GlobalConfig::GlobalConfig(QObject *parent)
    : QObject(parent)
{
    load();
}

QString GlobalConfig::ffmpegPath() const
{
    return m_ffmpegPath;
}
void GlobalConfig::setFfmpegPath(const QString &path)
{
    if (m_ffmpegPath != path) {
        m_ffmpegPath = path;
        emit ffmpegPathChanged();
        save();
    }
}

void GlobalConfig::load()
{
    QSettings &s = pluginGroupSettings("GlobalConfig");
    // ffmpeg
    m_ffmpegPath = s.value("ffmpegPath").toString();
    if (m_ffmpegPath.isEmpty()) {
        QString detected = detectFfmpeg();
        if (!detected.isEmpty()) {
            m_ffmpegPath = detected;
            save();
        }
    }
}
void GlobalConfig::save()
{
    QSettings &s = pluginGroupSettings("GlobalConfig");
    // ffmpeg
    s.setValue("ffmpegPath", m_ffmpegPath);

    s.sync();
}
QString GlobalConfig::detectFfmpeg()
{
    QString bundled = QCoreApplication::applicationDirPath()
        + QStringLiteral("/third/ffmpeg/ffmpeg.exe");

    if (QFileInfo::exists(bundled))
        return QDir::toNativeSeparators(bundled);

    QString fromPath = QStandardPaths::findExecutable(
        QStringLiteral("ffmpeg"));
    if (!fromPath.isEmpty())
        return QDir::toNativeSeparators(fromPath);

    return {};
}

QString GlobalConfig::backupPath()
{
    return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        + QStringLiteral("/BYTools/backup");
}
QString GlobalConfig::cachePath()
{
    return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        + QStringLiteral("/BYTools/cache");
}
QString GlobalConfig::tempDir()
{
    return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
        + QStringLiteral("/BYTools/temp");
}
