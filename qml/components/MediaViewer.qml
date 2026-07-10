//MediaViewer
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Item {
    id: root

    Layout.minimumWidth: 420
    Layout.minimumHeight: 360

    property alias source: mediaPlayer.source
    property alias position: mediaPlayer.position
    property alias duration: mediaPlayer.duration
    property alias playbackState: mediaPlayer.playbackState
    property alias muted: audioOut.muted

    property int volume: 100
    onVolumeChanged: audioOut.volume = volume / 100.0

    property real speed: 1.0
    onSpeedChanged: mediaPlayer.playbackRate = speed

    property bool showControls: true
    property bool showPreviousNext: true
    property bool showSeekButtons: true
    property int seekStepMs: 5000

    // ── 主题支持 ──
    property string paletteGroup: ""
    property var controlsBgColor: undefined
    property var controlsTextColor: undefined
    property var sliderTrackColor: undefined
    property var sliderProgressColor: undefined
    property var sliderHandleColor: undefined

    readonly property var _p: themeManager.palette
    readonly property color _controlsBgColor: root.controlsBgColor !== undefined ? root.controlsBgColor : (paletteGroup ? (_p[paletteGroup + "_bgColor"] || "#D0000000") : "#D0000000")
    readonly property color _controlsTextColor: root.controlsTextColor !== undefined ? root.controlsTextColor : (paletteGroup ? (_p[paletteGroup + "_textColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _sliderTrackColor: root.sliderTrackColor !== undefined ? root.sliderTrackColor : (paletteGroup ? (_p[paletteGroup + "_sliderTrackColor"] || "#40ffffff") : "#40ffffff")
    readonly property color _sliderProgressColor: root.sliderProgressColor !== undefined ? root.sliderProgressColor : (paletteGroup ? (_p[paletteGroup + "_sliderProgressColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _sliderHandleColor: root.sliderHandleColor !== undefined ? root.sliderHandleColor : (paletteGroup ? (_p[paletteGroup + "_sliderHandleColor"] || "#FFFFFF") : "#FFFFFF")

    signal previousRequested
    signal nextRequested
    signal deleteRequested
    signal speedSelected(real speed)

    function play() {
        mediaPlayer.play();
    }
    function pause() {
        mediaPlayer.pause();
    }
    function stop() {
        mediaPlayer.stop();
    }

    function fmtTime(ms) {
        if (ms < 0)
            return "-" + fmtTime(-ms);
        var h = Math.floor(ms / 3600000);
        var m = Math.floor((ms % 3600000) / 60000);
        var s = Math.floor((ms % 60000) / 1000);
        return (h < 10 ? "0" : "") + h + ":" + (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s;
    }

    MediaPlayer {
        id: mediaPlayer
        videoOutput: videoOut
        audioOutput: audioOut
    }

    AudioOutput {
        id: audioOut
    }

    Shortcut {
        sequence: "Left"
        enabled: root.visible && root.showSeekButtons
        onActivated: mediaPlayer.position = Math.max(0, mediaPlayer.position - root.seekStepMs)
    }
    Shortcut {
        sequence: "Right"
        enabled: root.visible && root.showSeekButtons
        onActivated: mediaPlayer.position = Math.min(mediaPlayer.duration, mediaPlayer.position + root.seekStepMs)
    }
    Shortcut {
        sequence: "Up"
        enabled: root.visible
        onActivated: {
            root.volume = Math.min(100, root.volume + 5);
            audioOut.muted = false;
            volumeTip.show();
        }
    }
    Shortcut {
        sequence: "Down"
        enabled: root.visible
        onActivated: {
            var v = Math.max(0, root.volume - 5);
            root.volume = v;
            audioOut.muted = (v < 1);
            volumeTip.show();
        }
    }
    Shortcut {
        sequence: "Shift+Left"
        enabled: root.visible && root.showPreviousNext
        onActivated: root.previousRequested()
    }
    Shortcut {
        sequence: "Shift+Right"
        enabled: root.visible && root.showPreviousNext
        onActivated: root.nextRequested()
    }
    Shortcut {
        sequence: "Delete"
        enabled: root.visible
        onActivated: root.deleteRequested()
    }
    Shortcut {
        sequence: "Space"
        enabled: root.visible
        onActivated: {
            if (mediaPlayer.playbackState === MediaPlayer.PlayingState)
                mediaPlayer.pause();
            else
                mediaPlayer.play();
        }
    }

    VideoOutput {
        id: videoOut
        anchors.fill: parent
        anchors.bottomMargin: showControls ? controlBar.height : 0
        fillMode: VideoOutput.PreserveAspectFit
    }

    MouseArea {
        anchors.fill: videoOut
        enabled: mediaPlayer.source != ""
        onClicked: {
            if (mediaPlayer.playbackState === MediaPlayer.PlayingState)
                mediaPlayer.pause();
            else
                mediaPlayer.play();
        }
    }

    // ── Controls bar ──
    Rectangle {
        id: controlBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 60
        color: root._controlsBgColor
        visible: showControls && mediaPlayer.source != ""

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            anchors.topMargin: 6
            anchors.bottomMargin: 6
            spacing: 2

            // Row 1: Time + Progress + Duration
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: root.fmtTime(mediaPlayer.position)
                    color: root._controlsTextColor
                    font.pixelSize: 11
                    font.family: "Consolas, monospace"
                }

                Slider {
                    id: seekSlider
                    Layout.fillWidth: true
                    implicitHeight: 20
                    leftPadding: 0
                    rightPadding: 0
                    from: 0
                    to: (mediaPlayer.duration > 0) ? mediaPlayer.duration : 1
                    value: mediaPlayer.position
                    enabled: mediaPlayer.duration > 0
                    onMoved: {
                        mediaPlayer.position = value;
                    }

                    background: Rectangle {
                        x: seekSlider.leftPadding
                        y: seekSlider.availableHeight / 2 - height / 2
                        width: seekSlider.availableWidth
                        height: 4
                        radius: 2
                        color: root._sliderTrackColor

                        Rectangle {
                            width: seekSlider.visualPosition * parent.width
                            height: parent.height
                            radius: 2
                            color: root._sliderProgressColor
                        }
                    }

                    handle: Rectangle {
                        x: seekSlider.leftPadding + seekSlider.visualPosition * (seekSlider.availableWidth - width)
                        y: seekSlider.availableHeight / 2 - height / 2
                        width: 14
                        height: 14
                        radius: 7
                        color: root._sliderHandleColor
                        visible: seekSlider.pressed || seekSlider.hovered
                    }
                }

                Label {
                    text: root.fmtTime(mediaPlayer.duration)
                    color: root._controlsTextColor
                    font.pixelSize: 11
                    font.family: "Consolas, monospace"
                }
            }

            // Row 2: Transport + Volume
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                IconButton {
                    implicitWidth: 28
                    iconSource: "qrc:/icons/media_previous.svg"
                    tooltip: "上一个"
                    paletteGroup: "IconBtnEx"
                    visible: root.showPreviousNext
                    onClicked: root.previousRequested()
                }

                IconButton {
                    implicitWidth: 28
                    iconSource: "qrc:/icons/media_seekdec.svg"
                    tooltip: "快退"
                    paletteGroup: "IconBtnEx"
                    visible: root.showSeekButtons
                    onClicked: {
                        mediaPlayer.position = Math.max(0, mediaPlayer.position - root.seekStepMs);
                    }
                }

                IconButton {
                    implicitWidth: 32
                    property bool isPlaying: mediaPlayer.playbackState === MediaPlayer.PlayingState
                    iconSource: isPlaying ? "qrc:/icons/media_pause.svg" : "qrc:/icons/media_play.svg"
                    tooltip: isPlaying ? "暂停" : "播放"
                    paletteGroup: "IconBtnEx"
                    onClicked: {
                        if (mediaPlayer.playbackState === MediaPlayer.PlayingState)
                            mediaPlayer.pause();
                        else
                            mediaPlayer.play();
                    }
                }

                IconButton {
                    implicitWidth: 28
                    iconSource: "qrc:/icons/media_seekadd.svg"
                    tooltip: "快进"
                    paletteGroup: "IconBtnEx"
                    visible: root.showSeekButtons
                    onClicked: {
                        mediaPlayer.position = Math.min(mediaPlayer.duration, mediaPlayer.position + root.seekStepMs);
                    }
                }

                IconButton {
                    implicitWidth: 28
                    iconSource: "qrc:/icons/media_next.svg"
                    tooltip: "下一个"
                    paletteGroup: "IconBtnEx"
                    visible: root.showPreviousNext
                    onClicked: root.nextRequested()
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    id: volumeTip
                    visible: false
                    text: root.volume
                    color: root._controlsTextColor
                    font.pixelSize: 11
                    padding: 4
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignVCenter
                    background: Rectangle {
                        color: "#D0000000"
                        radius: 4
                        border.width: 1
                        border.color: root._controlsTextColor
                    }
                    Timer {
                        id: volumeTipTimer
                        interval: 1500
                        onTriggered: parent.visible = false
                    }
                    function show() {
                        text = root.volume;
                        visible = true;
                        volumeTipTimer.restart();
                    }
                }

                IconButton {
                    id: muteBtn
                    implicitWidth: 28
                    property bool _isMuted: audioOut.muted || root.volume < 1
                    iconSource: _isMuted ? "qrc:/icons/media_mute.svg" : "qrc:/icons/media_volume.svg"
                    tooltip: _isMuted ? "取消静音" : "静音"
                    paletteGroup: "IconBtnEx"
                    onClicked: {
                        if (audioOut.muted) {
                            audioOut.muted = false;
                            if (root.volume < 1) {
                                root.volume = 5;
                                volumeTip.show();
                            }
                        } else {
                            audioOut.muted = true;
                        }
                    }
                }

                Slider {
                    id: volumeSlider
                    Layout.preferredWidth: 100
                    implicitHeight: 20
                    from: 0
                    to: 100
                    value: root.volume
                    onMoved: {
                        root.volume = value;
                        audioOut.muted = (value < 1);
                        volumeTip.show();
                    }

                    background: Rectangle {
                        x: volumeSlider.leftPadding
                        y: volumeSlider.availableHeight / 2 - height / 2
                        width: volumeSlider.availableWidth
                        height: 4
                        radius: 2
                        color: root._sliderTrackColor

                        Rectangle {
                            width: volumeSlider.visualPosition * parent.width
                            height: parent.height
                            radius: 2
                            color: root._sliderProgressColor
                        }
                    }

                    handle: Rectangle {
                        x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                        y: volumeSlider.availableHeight / 2 - height / 2
                        width: 14
                        height: 14
                        radius: 7
                        color: root._sliderHandleColor
                        visible: volumeSlider.pressed || volumeSlider.hovered
                    }
                }

                Item {
                    Layout.preferredWidth: 4
                }

                Row {
                    visible: root.showSeekButtons
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 4
                    Repeater {
                        model: [5, 10, 30]
                        delegate: Rectangle {
                            required property int modelData
                            width: 26
                            height: 20
                            color: "transparent"
                            border.width: root.seekStepMs === modelData * 1000 ? 1 : 0
                            border.color: root._controlsTextColor
                            radius: 3
                            ToolTip.visible: mouseArea.containsMouse
                            ToolTip.delay: 500
                            ToolTip.text: "快进/快退 " + modelData + " 秒"
                            Label {
                                anchors.centerIn: parent
                                text: modelData + "s"
                                color: root._controlsTextColor
                                font.pixelSize: 10
                            }
                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: root.seekStepMs = modelData * 1000
                            }
                        }
                    }
                }

                // ── 倍速调节 ──
                Row {
                    height: 22
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 2

                    Item {
                        width: 18
                        height: parent.height

                        Label {
                            anchors.centerIn: parent
                            text: "◀"
                            color: decMouse.containsMouse ? "#FFFFFF" : root._controlsTextColor
                            font.pixelSize: 18
                            font.bold: true
                        }

                        MouseArea {
                            id: decMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                var s = root.speed;
                                if (s <= 1.0)
                                    s = Math.max(0.1, Math.round((s - 0.1) * 10) / 10);
                                else
                                    s = Math.max(1.0, s - 1);
                                root.speed = s;
                                root.speedSelected(s);
                            }
                        }

                        ToolTip.visible: decMouse.containsMouse
                        ToolTip.delay: 500
                        ToolTip.text: "减速"
                    }

                    Item {
                        width: 30
                        height: parent.height

                        Label {
                            anchors.centerIn: parent
                            text: root.speed.toFixed(1) + "x"
                            color: root._controlsTextColor
                            font.pixelSize: 10
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        MouseArea {
                            id: speedResetArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onDoubleClicked: {
                                root.speed = 1.0;
                                root.speedSelected(1.0);
                            }
                        }

                        ToolTip.visible: speedResetArea.containsMouse
                        ToolTip.delay: 500
                        ToolTip.text: "双击还原为1.0x"
                    }

                    Item {
                        width: 18
                        height: parent.height

                        Label {
                            anchors.centerIn: parent
                            text: "▶"
                            color: incMouse.containsMouse ? "#FFFFFF" : root._controlsTextColor
                            font.pixelSize: 18
                            font.bold: true
                        }

                        MouseArea {
                            id: incMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                var s = root.speed;
                                if (s < 1.0)
                                    s = Math.min(1.0, Math.round((s + 0.1) * 10) / 10);
                                else
                                    s = Math.min(5.0, s + 1);
                                root.speed = s;
                                root.speedSelected(s);
                            }
                        }

                        ToolTip.visible: incMouse.containsMouse
                        ToolTip.delay: 500
                        ToolTip.text: "加速"
                    }
                }
            }
        }
    }

    onVisibleChanged: {
        if (!root.visible && mediaPlayer.playbackState === MediaPlayer.PlayingState)
            mediaPlayer.pause();
    }
}
