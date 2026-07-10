#include "MpvPlayer.h"
#include <QQuickWindow>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QFileInfo>
#include <QCoreApplication>
#include <QtMath>
#include <QDebug>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

MpvPlayer::MpvPlayer(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, false);

    m_connectTimer = new QTimer(this);
    m_connectTimer->setSingleShot(true);
    m_connectTimer->setInterval(300);
    connect(m_connectTimer, &QTimer::timeout, this, &MpvPlayer::connectIpc);
}

MpvPlayer::~MpvPlayer()
{
    stopMpv();
    destroyChildWindow();
}

void MpvPlayer::setSource(const QUrl &source)
{
    if (m_source == source && !source.isEmpty())
        return;
    m_source = source;
    emit sourceChanged();
    stopMpv();
    if (!source.isLocalFile())
        return;
    if (m_mpvPath.isEmpty() || !QFileInfo::exists(m_mpvPath))
        return;
    createChildWindow();
    startMpv();
}

void MpvPlayer::setPosition(qint64 pos)
{
    if (pos < 0) pos = 0;
    m_position = pos;
    emit positionChanged();
    seek(pos);
}

// mpv 内部 volume 使用三次曲线：gain = (vol/100)^3
// 导致 0-30 区间几乎没有增益变化，在此做逆补偿使 UI 滑块感知线性
static qreal linearizeMpvVolume(qreal uiVol)
{
    return qPow(uiVol / 100.0, 1.0 / 3.0) * 100.0;
}

void MpvPlayer::setVolume(qreal vol)
{
    vol = qBound<qreal>(0, vol, 100);
    if (qFuzzyCompare(m_volume, vol))
        return;
    m_volume = vol;
    emit volumeChanged();
    if (m_ipcSocket && m_ipcSocket->state() == QLocalSocket::ConnectedState) {
        QJsonObject cmd;
        QJsonArray args;
        args.append(QStringLiteral("set_property"));
        args.append(QStringLiteral("volume"));
        args.append(linearizeMpvVolume(vol));
        cmd[QStringLiteral("command")] = args;
        sendIpcCommand(QJsonDocument(cmd).toJson(QJsonDocument::Compact));
    }
}

void MpvPlayer::setSpeed(qreal speed)
{
    speed = qBound<qreal>(0.1, speed, 10.0);
    if (qFuzzyCompare(m_speed, speed))
        return;
    m_speed = speed;
    emit speedChanged();
    if (m_ipcSocket && m_ipcSocket->state() == QLocalSocket::ConnectedState) {
        QJsonObject cmd;
        QJsonArray args;
        args.append(QStringLiteral("set_property"));
        args.append(QStringLiteral("speed"));
        args.append(speed);
        cmd[QStringLiteral("command")] = args;
        sendIpcCommand(QJsonDocument(cmd).toJson(QJsonDocument::Compact));
    }
}

void MpvPlayer::setMuted(bool muted)
{
    if (m_muted == muted)
        return;
    m_muted = muted;
    emit mutedChanged();
    if (m_ipcSocket && m_ipcSocket->state() == QLocalSocket::ConnectedState) {
        QJsonObject cmd;
        QJsonArray args;
        args.append(QStringLiteral("set_property"));
        args.append(QStringLiteral("mute"));
        args.append(QJsonValue(muted));
        cmd[QStringLiteral("command")] = args;
        sendIpcCommand(QJsonDocument(cmd).toJson(QJsonDocument::Compact));
    }
}

void MpvPlayer::setMpvPath(const QString &path)
{
    if (m_mpvPath == path)
        return;
    m_mpvPath = path;
    m_available = !path.isEmpty() && QFileInfo::exists(path);
    emit mpvPathChanged();
    emit availableChanged();
}

void MpvPlayer::play()
{
    m_pendingPlay = false;
    m_pendingPause = false;
    if (m_ipcSocket && m_ipcSocket->state() == QLocalSocket::ConnectedState) {
        QJsonObject cmd;
        QJsonArray args;
        args.append(QStringLiteral("cycle"));
        args.append(QStringLiteral("pause"));
        cmd[QStringLiteral("command")] = args;
        sendIpcCommand(QJsonDocument(cmd).toJson(QJsonDocument::Compact));
    } else {
        m_pendingPlay = true;
    }
}

void MpvPlayer::pause()
{
    m_pendingPlay = false;
    m_pendingPause = false;
    if (m_ipcSocket && m_ipcSocket->state() == QLocalSocket::ConnectedState) {
        QJsonObject cmd;
        QJsonArray args;
        args.append(QStringLiteral("cycle"));
        args.append(QStringLiteral("pause"));
        cmd[QStringLiteral("command")] = args;
        sendIpcCommand(QJsonDocument(cmd).toJson(QJsonDocument::Compact));
    } else {
        m_pendingPause = true;
    }
}

void MpvPlayer::stop()
{
    stopMpv();
    m_position = 0;
    m_duration = 0;
    m_playbackState = Stopped;
    emit positionChanged();
    emit durationChanged();
    emit playbackStateChanged();
}

void MpvPlayer::seek(qint64 positionMs)
{
    if (m_ipcSocket && m_ipcSocket->state() == QLocalSocket::ConnectedState) {
        double sec = positionMs / 1000.0;
        QJsonObject cmd;
        cmd[QStringLiteral("command")] = QJsonArray{
            QStringLiteral("seek"), sec, QStringLiteral("absolute")
        };
        sendIpcCommand(QJsonDocument(cmd).toJson(QJsonDocument::Compact));
    }
}

// ── QQuickItem overrides ──

void MpvPlayer::itemChange(ItemChange change, const ItemChangeData &data)
{
    if (change == ItemSceneChange && data.window) {
        if (m_childWindow)
            updateWindowGeometry();
    }
    if (change == ItemVisibleHasChanged && !data.boolValue) {
        if (m_process && m_playbackState == Playing)
            pause();
    }
    QQuickItem::itemChange(change, data);
}

void MpvPlayer::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    updateWindowGeometry();
}

// ── Window management ──

void MpvPlayer::createChildWindow()
{
    if (m_childWindow)
        return;
    if (!window())
        return;
    m_childWindow = new QWindow(window());
    m_childWindow->setFlags(Qt::FramelessWindowHint | Qt::WindowTransparentForInput);
    m_childWindow->setGeometry(mapRectToScene(QRectF(QPointF(0, 0), size())).toRect());
    m_childWindow->show();

    // Paint background black so the video area isn't white before mpv draws
    paintWindowBlack();
}

void MpvPlayer::paintWindowBlack()
{
    if (!m_childWindow || !m_childWindow->isVisible())
        return;
    HWND hwnd = reinterpret_cast<HWND>(m_childWindow->winId());
    HDC hdc = GetDC(hwnd);
    RECT r;
    GetClientRect(hwnd, &r);
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &r, brush);
    DeleteObject(brush);
    ReleaseDC(hwnd, hdc);
}

void MpvPlayer::destroyChildWindow()
{
    if (!m_childWindow)
        return;
    m_childWindow->hide();
    m_childWindow->deleteLater();
    m_childWindow = nullptr;
}

void MpvPlayer::setNativeOverlayVisible(bool visible)
{
    if (!m_childWindow)
        return;
    if (visible)
        m_childWindow->show();
    else
        m_childWindow->hide();
}

void MpvPlayer::updateWindowGeometry()
{
    if (!m_childWindow)
        return;
    if (!window())
        return;
    QRect r = mapRectToScene(QRectF(QPointF(0, 0), size())).toRect();
    if (r.width() < 1 || r.height() < 1)
        return;
    m_childWindow->setGeometry(r);
    paintWindowBlack();
}

// ── mpv process management ──

void MpvPlayer::startMpv()
{
    if (m_process)
        stopMpv();
    if (!m_childWindow || !m_source.isLocalFile())
        return;

    QString filePath = m_source.toLocalFile();
    if (!QFileInfo::exists(filePath))
        return;

    // Generate unique pipe name
    int r = QRandomGenerator::global()->bounded(100000, 999999);
    m_pipeName = QStringLiteral("mpv-bytools-%1").arg(r);

    // Build full IPC path: \\.\pipe\mpv-bytools-XXXXX
    QString ipcPath = QStringLiteral(R"(\\.\pipe\)") + m_pipeName;

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::ForwardedChannels);

    HWND hwnd = reinterpret_cast<HWND>(m_childWindow->winId());
    QStringList args;
    args << QStringLiteral("--wid=%1").arg(reinterpret_cast<qintptr>(hwnd));
    args << QStringLiteral("--input-ipc-server=%1").arg(ipcPath);
    args << QStringLiteral("--no-border");
    args << QStringLiteral("--no-osc");
    args << QStringLiteral("--no-input-default-bindings");
    args << QStringLiteral("--no-input-cursor");
    args << QStringLiteral("--cursor-autohide=no");
    args << QStringLiteral("--keepaspect=yes");
    args << QStringLiteral("--volume=%1").arg(static_cast<int>(linearizeMpvVolume(m_volume)));
    if (m_muted)
        args << QStringLiteral("--mute=yes");
    args << QStringLiteral("--pause");
    args << QStringLiteral("--no-terminal");
    args << QStringLiteral("--quiet");
    args << QStringLiteral("--sub-auto=no");
    args << filePath;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    m_process->setProcessEnvironment(env);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(exitCode);
        Q_UNUSED(status);
        if (m_playbackState != Stopped) {
            m_playbackState = Stopped;
            emit playbackStateChanged();
        }
    });

    m_process->start(m_mpvPath, args);

    if (!m_process->waitForStarted(3000)) {
        emit errorOccurred(QStringLiteral("mpv failed to start: ") + m_process->errorString());
        stopMpv();
        return;
    }

    // Start polling for IPC connection
    m_connectTimer->start();
}

void MpvPlayer::stopMpv()
{
    m_connectTimer->stop();
    disconnectIpc();
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_playbackState = Stopped;
    m_position = 0;
    m_duration = 0;
    emit playbackStateChanged();
    emit positionChanged();
    emit durationChanged();
    destroyChildWindow();
}

// ── IPC (Named Pipe) ──

void MpvPlayer::connectIpc()
{
    if (m_ipcSocket)
        disconnectIpc();
    if (!m_process || m_process->state() == QProcess::NotRunning)
        return;

    m_ipcSocket = new QLocalSocket(this);
    connect(m_ipcSocket, &QLocalSocket::readyRead, this, &MpvPlayer::handleIpcData);
    connect(m_ipcSocket, &QLocalSocket::connected, this, [this]() {
        observeProperties();

        // Re-apply volume and mute in case they were set before IPC was ready
        QJsonObject volCmd;
        QJsonArray volArgs;
        volArgs.append(QStringLiteral("set_property"));
        volArgs.append(QStringLiteral("volume"));
        volArgs.append(linearizeMpvVolume(m_volume));
        volCmd[QStringLiteral("command")] = volArgs;
        sendIpcCommand(QJsonDocument(volCmd).toJson(QJsonDocument::Compact));

        if (m_muted) {
            QJsonObject muteCmd;
            QJsonArray muteArgs;
            muteArgs.append(QStringLiteral("set_property"));
            muteArgs.append(QStringLiteral("mute"));
            muteArgs.append(QJsonValue(true));
            muteCmd[QStringLiteral("command")] = muteArgs;
            sendIpcCommand(QJsonDocument(muteCmd).toJson(QJsonDocument::Compact));
        }

        // Re-apply speed
        QJsonObject speedCmd;
        QJsonArray speedArgs;
        speedArgs.append(QStringLiteral("set_property"));
        speedArgs.append(QStringLiteral("speed"));
        speedArgs.append(m_speed);
        speedCmd[QStringLiteral("command")] = speedArgs;
        sendIpcCommand(QJsonDocument(speedCmd).toJson(QJsonDocument::Compact));
    });
    connect(m_ipcSocket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError err) {
        if (err != QLocalSocket::PeerClosedError) {
            if (m_connectTimer && !m_connectTimer->isActive())
                m_connectTimer->start();
        }
    });

    m_ipcSocket->connectToServer(m_pipeName);
}

void MpvPlayer::disconnectIpc()
{
    if (m_ipcSocket) {
        m_ipcSocket->disconnectFromServer();
        m_ipcSocket->deleteLater();
        m_ipcSocket = nullptr;
    }
    m_ipcBuffer.clear();
}

void MpvPlayer::sendIpcCommand(const QByteArray &jsonCmd)
{
    if (!m_ipcSocket || m_ipcSocket->state() != QLocalSocket::ConnectedState)
        return;
    QByteArray msg = jsonCmd + '\n';
    m_ipcSocket->write(msg);
    m_ipcSocket->flush();
}

void MpvPlayer::handleIpcData()
{
    m_ipcBuffer.append(m_ipcSocket->readAll());

    int idx;
    while ((idx = m_ipcBuffer.indexOf('\n')) >= 0) {
        QByteArray line = m_ipcBuffer.left(idx).trimmed();
        m_ipcBuffer.remove(0, idx + 1);
        if (line.isEmpty())
            continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject())
            continue;

        QJsonObject obj = doc.object();

        if (obj.contains(QStringLiteral("event"))) {
            QString event = obj[QStringLiteral("event")].toString();
            if (event == QStringLiteral("property-change")) {
                QString name = obj[QStringLiteral("name")].toString();
                if (name == QStringLiteral("time-pos")) {
                    double pos = obj[QStringLiteral("data")].toDouble(0);
                    qint64 newPos = static_cast<qint64>(pos * 1000);
                    if (newPos != m_position) {
                        m_position = newPos;
                        emit positionChanged();
                    }
                } else if (name == QStringLiteral("duration")) {
                    double dur = obj[QStringLiteral("data")].toDouble(0);
                    qint64 newDur = static_cast<qint64>(dur * 1000);
                    if (newDur != m_duration) {
                        m_duration = newDur;
                        emit durationChanged();
                    }
                } else if (name == QStringLiteral("pause")) {
                    bool paused = obj[QStringLiteral("data")].toBool();
                    PlaybackState newState = paused ? Paused : Playing;
                    if (newState != m_playbackState) {
                        m_playbackState = newState;
                        emit playbackStateChanged();
                    }
                    // Handle pending play/pause on first observation
                    if (paused && m_pendingPlay) {
                        m_pendingPlay = false;
                        m_pendingPause = false;
                        QJsonObject cmd;
                        QJsonArray args;
                        args.append(QStringLiteral("cycle"));
                        args.append(QStringLiteral("pause"));
                        cmd[QStringLiteral("command")] = args;
                        sendIpcCommand(QJsonDocument(cmd).toJson(QJsonDocument::Compact));
                    } else if (!paused && m_pendingPause) {
                        m_pendingPause = false;
                        m_pendingPlay = false;
                        QJsonObject cmd;
                        QJsonArray args;
                        args.append(QStringLiteral("cycle"));
                        args.append(QStringLiteral("pause"));
                        cmd[QStringLiteral("command")] = args;
                        sendIpcCommand(QJsonDocument(cmd).toJson(QJsonDocument::Compact));
                    }
                    m_pendingPlay = false;
                    m_pendingPause = false;
                } else if (name == QStringLiteral("eof-reached")) {
                    m_playbackState = Stopped;
                    m_position = m_duration > 0 ? m_duration : 0;
                    emit playbackStateChanged();
                    emit positionChanged();
                }
            } else if (event == QStringLiteral("end-file")) {
                m_playbackState = Stopped;
                emit playbackStateChanged();
            }
        }

        if (obj.contains(QStringLiteral("error"))) {
            QString error = obj[QStringLiteral("error")].toString();
            if (error != QStringLiteral("success")) {
                qDebug() << "mpv IPC error:" << error;
            }
        }
    }
}

void MpvPlayer::observeProperties()
{
    struct { int id; const char *prop; } props[] = {
        {1, "time-pos"},
        {2, "duration"},
        {3, "pause"},
        {4, "eof-reached"},
    };

    for (const auto &p : props) {
        QJsonObject cmd;
        QJsonArray args;
        args.append(QStringLiteral("observe_property"));
        args.append(p.id);
        args.append(QString::fromLatin1(p.prop));
        cmd[QStringLiteral("command")] = args;
        sendIpcCommand(QJsonDocument(cmd).toJson(QJsonDocument::Compact));
    }
}
