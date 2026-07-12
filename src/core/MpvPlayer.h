#pragma once

#include <QQuickItem>
#include <QProcess>
#include <QWindow>
#include <QUrl>
#include <QLocalSocket>
#include <QTimer>

class MpvPlayer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
    Q_PROPERTY(QString mpvPath READ mpvPath WRITE setMpvPath NOTIFY mpvPathChanged)

public:
    enum PlaybackState { Stopped = 0, Playing = 1, Paused = 2 };
    Q_ENUM(PlaybackState)

    explicit MpvPlayer(QQuickItem *parent = nullptr);
    ~MpvPlayer() override;

    QUrl source() const { return m_source; }
    void setSource(const QUrl &source);

    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    PlaybackState playbackState() const { return m_playbackState; }
    qreal volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    bool isAvailable() const { return m_available; }
    QString mpvPath() const { return m_mpvPath; }

    void setPosition(qint64 pos);
    void setVolume(qreal vol);
    void setMuted(bool muted);
    qreal speed() const { return m_speed; }
    void setSpeed(qreal speed);
    void setMpvPath(const QString &path);

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void setNativeOverlayVisible(bool visible);

signals:
    void sourceChanged();
    void positionChanged();
    void durationChanged();
    void playbackStateChanged();
    void volumeChanged();
    void mutedChanged();
    void speedChanged();
    void availableChanged();
    void mpvPathChanged();
    void errorOccurred(const QString &error);
    void finished();

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void createChildWindow();
    void destroyChildWindow();
    void updateWindowGeometry();
    void paintWindowBlack();
    void startMpv();
    void stopMpv();
    void connectIpc();
    void disconnectIpc();
    void sendIpcCommand(const QByteArray &jsonCmd);
    void handleIpcData();
    void observeProperties();

    QProcess *m_process = nullptr;
    QWindow *m_childWindow = nullptr;
    QLocalSocket *m_ipcSocket = nullptr;
    QTimer *m_connectTimer = nullptr;

    QString m_pipeName;
    QByteArray m_ipcBuffer;

    bool m_pendingPlay = false;
    bool m_pendingPause = false;

    QUrl m_source;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    PlaybackState m_playbackState = Stopped;
    qreal m_volume = 100;
    bool m_muted = false;
    qreal m_speed = 1.0;
    bool m_available = false;
    QString m_mpvPath;
};
