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

    property bool showControls: true
    property bool showPreviousNext: true
    property bool showSeekButtons: true
    property int seekStepMs: 5000

    // ── 主题支持 ──
    property string controlsPaletteGroup: ""
    property color controlsBgColor: "#D0000000"
    property color controlsTextColor: "#ffffff"
    property color sliderTrackColor: "#40ffffff"
    property color sliderProgressColor: "#ffffff"
    property color sliderHandleColor: "#ffffff"

    readonly property var _p: themeManager.palette
    readonly property color _controlsBgColor:
        controlsPaletteGroup ? (_p[controlsPaletteGroup + "_bgColor"] || controlsBgColor) : controlsBgColor
    readonly property color _controlsTextColor:
        controlsPaletteGroup ? (_p[controlsPaletteGroup + "_textColor"] || controlsTextColor) : controlsTextColor
    readonly property color _sliderTrackColor:
        controlsPaletteGroup ? (_p[controlsPaletteGroup + "_sliderTrackColor"] || sliderTrackColor) : sliderTrackColor
    readonly property color _sliderProgressColor:
        controlsPaletteGroup ? (_p[controlsPaletteGroup + "_sliderProgressColor"] || sliderProgressColor) : sliderProgressColor
    readonly property color _sliderHandleColor:
        controlsPaletteGroup ? (_p[controlsPaletteGroup + "_sliderHandleColor"] || sliderHandleColor) : sliderHandleColor

    signal previousRequested()
    signal nextRequested()
    signal deleteRequested()

    function play() { mpvPlayer.play() }
    function pause() { mpvPlayer.pause() }
    function stop() { mpvPlayer.stop() }
    function setNativeOverlayVisible(v) { mpvPlayer.setNativeOverlayVisible(v) }


    function fmtTime(ms) {
        if (ms < 0) return "-" + fmtTime(-ms)
        var h = Math.floor(ms / 3600000)
        var m = Math.floor((ms % 3600000) / 60000)
        var s = Math.floor((ms % 60000) / 1000)
        return (h < 10 ? "0" : "") + h + ":"
             + (m < 10 ? "0" : "") + m + ":"
             + (s < 10 ? "0" : "") + s
    }

    // ── Loading overlay ──
    property bool _hasSource: mpvPlayer.source.toString().length > 0
    property bool _loading: _hasSource && mpvPlayer.playbackState !== MpvPlayer.Playing
                         && mpvPlayer.playbackState !== MpvPlayer.Paused

    MpvPlayer {
        id: mpvPlayer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: showControls ? controlBar.top : parent.bottom
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
                border.color: "#ffffff"

                Rectangle {
                    anchors.centerIn: parent
                    width: 30
                    height: 30
                    radius: 15
                    color: "transparent"
                    border.width: 3
                    border.color: "#60a5fa"

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
                color: "#ffffff"
                font.pixelSize: 14
            }
        }
    }

    Shortcut { sequence: "Left";       enabled: root.visible && root.showSeekButtons;     onActivated: mpvPlayer.position = Math.max(0, mpvPlayer.position - root.seekStepMs) }
    Shortcut { sequence: "Right";      enabled: root.visible && root.showSeekButtons;     onActivated: mpvPlayer.position = Math.min(mpvPlayer.duration, mpvPlayer.position + root.seekStepMs) }
    Shortcut { sequence: "Up";         enabled: root.visible;                             onActivated: { root.volume = Math.min(100, root.volume + 5); mpvPlayer.muted = false; volumeTip.show() } }
    Shortcut { sequence: "Down";       enabled: root.visible;                             onActivated: { var v = Math.max(0, root.volume - 5); root.volume = v; mpvPlayer.muted = (v < 1); volumeTip.show() } }
    Shortcut { sequence: "Shift+Left"; enabled: root.visible && root.showPreviousNext;    onActivated: root.previousRequested() }
    Shortcut { sequence: "Shift+Right";enabled: root.visible && root.showPreviousNext;    onActivated: root.nextRequested() }
    Shortcut { sequence: "Delete";      enabled: root.visible;                             onActivated: root.deleteRequested() }

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
                        mpvPlayer.position = value
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
                    iconSource: "qrc:/icons/media-previous.svg"
                    tooltip: "上一个"
                    paletteGroup: root.controlsPaletteGroup
                    visible: root.showPreviousNext
                    onClicked: root.previousRequested()
                }

                IconButton {
                    implicitWidth: 28
                    iconSource: "qrc:/icons/media-seekdec.svg"
                    tooltip: "快退"
                    paletteGroup: root.controlsPaletteGroup
                    visible: root.showSeekButtons
                    onClicked: {
                        mpvPlayer.position = Math.max(0, mpvPlayer.position - root.seekStepMs)
                    }
                }

                IconButton {
                    implicitWidth: 32
                    property bool isPlaying: mpvPlayer.playbackState === MpvPlayer.Playing
                    iconSource: isPlaying ? "qrc:/icons/media-pause.svg" : "qrc:/icons/media-play.svg"
                    tooltip: isPlaying ? "暂停" : "播放"
                    paletteGroup: root.controlsPaletteGroup
                    onClicked: {
                        if (mpvPlayer.playbackState === MpvPlayer.Playing)
                            mpvPlayer.pause()
                        else
                            mpvPlayer.play()
                    }
                }

                IconButton {
                    implicitWidth: 28
                    iconSource: "qrc:/icons/media-seekadd.svg"
                    tooltip: "快进"
                    paletteGroup: root.controlsPaletteGroup
                    visible: root.showSeekButtons
                    onClicked: {
                        mpvPlayer.position = Math.min(mpvPlayer.duration, mpvPlayer.position + root.seekStepMs)
                    }
                }

                IconButton {
                    implicitWidth: 28
                    iconSource: "qrc:/icons/media-next.svg"
                    tooltip: "下一个"
                    paletteGroup: root.controlsPaletteGroup
                    visible: root.showPreviousNext
                    onClicked: root.nextRequested()
                }

                Item { Layout.fillWidth: true }

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
                        text = root.volume
                        visible = true
                        volumeTipTimer.restart()
                    }
                }

                IconButton {
                    id: muteBtn
                    implicitWidth: 28
                    property bool _isMuted: mpvPlayer.muted || root.volume < 1
                    iconSource: _isMuted ? "qrc:/icons/media-mute.svg" : "qrc:/icons/media-volume.svg"
                    tooltip: _isMuted ? "取消静音" : "静音"
                    paletteGroup: root.controlsPaletteGroup
                    onClicked: {
                        if (mpvPlayer.muted) {
                            mpvPlayer.muted = false
                            if (root.volume < 1) {
                                root.volume = 5
                                volumeTip.show()
                            }
                        } else {
                            mpvPlayer.muted = true
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
                        root.volume = value
                        mpvPlayer.muted = (value < 1)
                        volumeTip.show()
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

                Item { Layout.preferredWidth: 4 }

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
            }
        }
    }

    onVisibleChanged: {
        if (!root.visible && mpvPlayer.playbackState === MpvPlayer.Playing)
            mpvPlayer.pause()
    }
}
