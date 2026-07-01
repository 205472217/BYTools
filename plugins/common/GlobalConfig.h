#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class GlobalConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ffmpegPath READ ffmpegPath WRITE setFfmpegPath NOTIFY ffmpegPathChanged)

public:
    static GlobalConfig *instance();

    QString ffmpegPath() const;
    void setFfmpegPath(const QString &path);

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();
    Q_INVOKABLE static QString detectFfmpeg();

signals:
    void ffmpegPathChanged();

private:
    explicit GlobalConfig(QObject *parent = nullptr);

    QString m_ffmpegPath;
};
