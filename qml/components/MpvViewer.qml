import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import BYTools

Item {
    id: root

    Layout.minimumWidth: 420
    Layout.minimumHeight: 360

    property alias source: mpvPlayer.source
    property alias position: mpvPlayer.position
    property alias duration: mpvPlayer.duration
    property alias playbackState: mpvPlayer.playbackState
    property alias muted: mpvPlayer.muted
    property alias mpvPath: mpvPlayer.mpvPath

    property int volume: 100
    onVolumeChanged: mpvPlayer.volume = volume

    property real speed: mpvPlayer.speed
    onSpeedChanged: mpvPlayer.speed = speed

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
        mpvPlayer.play();
    }
    function pause() {
        mpvPlayer.pause();
    }
    function stop() {
        mpvPlayer.stop();
    }
    function setNativeOverlayVisible(v) {
        mpvPlayer.setNativeOverlayVisible(v);
    }

    function fmtTime(ms) {
        if (ms < 0)
            return "-" + fmtTime(-ms);
        var h = Math.floor(ms / 3600000);
        var m = Math.floor((ms % 3600000) / 60000);
        var s = Math.floor((ms % 60000) / 1000);
        return (h < 10 ? "0" : "") + h + ":" + (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s;
    }

    // ── Loading overlay ──
    property bool _hasSource: mpvPlayer.source.toString().length > 0
    property bool _loading: _hasSource && mpvPlayer.playbackState !== MpvPlayer.Playing && mpvPlayer.playbackState !== MpvPlayer.Paused

    MpvPlayer {
        id: mpvPlayer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: showControls ? controlBar.top : parent.bottom
    }

    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: showControls ? controlBar.top : parent.bottom
        enabled: mpvPlayer.source != ""
        onClicked: {
            if (mpvPlayer.playbackState === MpvPlayer.Playing)
                mpvPlayer.pause();
            else
                mpvPlayer.play();
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: showControls ? controlBar.top : parent.bottom
        color: "#CC000000"
        visible: _loading

        Column {
            anchors.centerIn: parent
            spacing: 16

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 40
                height: 40
                radius: 20
                color: "transparent"
                border.width: 3
                border.color: "#FFFFFF"

                Rectangle {
                    anchors.centerIn: parent
                    width: 30
                    height: 30
                    radius: 15
                    color: "transparent"
                    border.width: 3
                    border.color: "#60A5FA"

                    NumberAnimation on rotation {
                        from: 0
                        to: 360
                        duration: 1000
                        loops: Animation.Infinite
                    }
                }
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "正在加载..."
                color: "#FFFFFF"
                font.pixelSize: 14
            }
        }
    }

    Shortcut {
        sequence: "Left"
        enabled: root.visible && root.showSeekButtons
        onActivated: mpvPlayer.position = Math.max(0, mpvPlayer.position - root.seekStepMs)
    }
    Shortcut {
        sequence: "Right"
        enabled: root.visible && root.showSeekButtons
        onActivated: mpvPlayer.position = Math.min(mpvPlayer.duration, mpvPlayer.position + root.seekStepMs)
    }
    Shortcut {
        sequence: "Up"
        enabled: root.visible
        onActivated: {
            root.volume = Math.min(100, root.volume + 5);
            mpvPlayer.muted = false;
            volumeTip.show();
        }
    }
    Shortcut {
        sequence: "Down"
        enabled: root.visible
        onActivated: {
            var v = Math.max(0, root.volume - 5);
            root.volume = v;
            mpvPlayer.muted = (v < 1);
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
            if (mpvPlayer.playbackState === MpvPlayer.Playing)
                mpvPlayer.pause();
            else
                mpvPlayer.play();
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
        visible: showControls && mpvPlayer.source != ""

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
                    text: root.fmtTime(mpvPlayer.position)
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
                    to: (mpvPlayer.duration > 0) ? mpvPlayer.duration : 1
                    value: mpvPlayer.position
                    enabled: mpvPlayer.duration > 0
                    onMoved: {
                        mpvPlayer.position = value;
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
                    text: root.fmtTime(mpvPlayer.duration)
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
                        mpvPlayer.position = Math.max(0, mpvPlayer.position - root.seekStepMs);
                    }
                }

                IconButton {
                    implicitWidth: 32
                    property bool isPlaying: mpvPlayer.playbackState === MpvPlayer.Playing
                    iconSource: isPlaying ? "qrc:/icons/media_pause.svg" : "qrc:/icons/media_play.svg"
                    tooltip: isPlaying ? "暂停" : "播放"
                    paletteGroup: "IconBtnEx"
                    onClicked: {
                        if (mpvPlayer.playbackState === MpvPlayer.Playing)
                            mpvPlayer.pause();
                        else
                            mpvPlayer.play();
                    }
                }

                IconButton {
                    implicitWidth: 28
                    iconSource: "qrc:/icons/media_seekadd.svg"
                    tooltip: "快进"
                    paletteGroup: "IconBtnEx"
                    visible: root.showSeekButtons
                    onClicked: {
                        mpvPlayer.position = Math.min(mpvPlayer.duration, mpvPlayer.position + root.seekStepMs);
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
                    property bool _isMuted: mpvPlayer.muted || root.volume < 1
                    iconSource: _isMuted ? "qrc:/icons/media_mute.svg" : "qrc:/icons/media_volume.svg"
                    tooltip: _isMuted ? "取消静音" : "静音"
                    paletteGroup: "IconBtnEx"
                    onClicked: {
                        if (mpvPlayer.muted) {
                            mpvPlayer.muted = false;
                            if (root.volume < 1) {
                                root.volume = 5;
                                volumeTip.show();
                            }
                        } else {
                            mpvPlayer.muted = true;
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
                        mpvPlayer.muted = (value < 1);
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
        if (!root.visible && mpvPlayer.playbackState === MpvPlayer.Playing)
            mpvPlayer.pause();
    }
}
