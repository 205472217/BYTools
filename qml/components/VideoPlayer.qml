//VideoPlayer
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Item {
    id: root

    property alias source: mediaPlayer.source
    property alias position: mediaPlayer.position
    property alias duration: mediaPlayer.duration
    property alias playbackState: mediaPlayer.playbackState
    property alias volume: audioOut.volume
    property alias muted: audioOut.muted

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

    function play() { mediaPlayer.play() }
    function pause() { mediaPlayer.pause() }
    function stop() { mediaPlayer.stop() }

    function fmtTime(ms) {
        if (ms < 0) return "-" + fmtTime(-ms)
        var h = Math.floor(ms / 3600000)
        var m = Math.floor((ms % 3600000) / 60000)
        var s = Math.floor((ms % 60000) / 1000)
        return (h < 10 ? "0" : "") + h + ":"
             + (m < 10 ? "0" : "") + m + ":"
             + (s < 10 ? "0" : "") + s
    }

    MediaPlayer {
        id: mediaPlayer
        videoOutput: videoOut
        audioOutput: audioOut
    }

    AudioOutput {
        id: audioOut
    }

    VideoOutput {
        id: videoOut
        anchors.fill: parent
        anchors.bottomMargin: showControls ? controlBar.height : 0
        fillMode: VideoOutput.PreserveAspectFit
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
                        mediaPlayer.position = value
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
                        mediaPlayer.position = Math.max(0, mediaPlayer.position - root.seekStepMs)
                    }
                }

                IconButton {
                    implicitWidth: 32
                    property bool isPlaying: mediaPlayer.playbackState === MediaPlayer.PlayingState
                    iconSource: isPlaying ? "qrc:/icons/media-pause.svg" : "qrc:/icons/media-play.svg"
                    tooltip: isPlaying ? "暂停" : "播放"
                    paletteGroup: root.controlsPaletteGroup
                    onClicked: {
                        if (mediaPlayer.playbackState === MediaPlayer.PlayingState)
                            mediaPlayer.pause()
                        else
                            mediaPlayer.play()
                    }
                }

                IconButton {
                    implicitWidth: 28
                    iconSource: "qrc:/icons/media-seekadd.svg"
                    tooltip: "快进"
                    paletteGroup: root.controlsPaletteGroup
                    visible: root.showSeekButtons
                    onClicked: {
                        mediaPlayer.position = Math.min(mediaPlayer.duration, mediaPlayer.position + root.seekStepMs)
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

                IconButton {
                    id: muteBtn
                    implicitWidth: 28
                    property bool isMuted: audioOut.muted
                    iconSource: isMuted ? "qrc:/icons/media-mute.svg" : "qrc:/icons/media-volume.svg"
                    tooltip: isMuted ? "取消静音" : "静音"
                    paletteGroup: root.controlsPaletteGroup
                    onClicked: {
                        audioOut.muted = !audioOut.muted
                    }
                }

                Slider {
                    id: volumeSlider
                    Layout.preferredWidth: 100
                    implicitHeight: 20
                    from: 0
                    to: 1
                    value: audioOut.volume
                    onMoved: {
                        audioOut.volume = value
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
            }
        }
    }

    onVisibleChanged: {
        if (!root.visible && mediaPlayer.playbackState === MediaPlayer.PlayingState)
            mediaPlayer.pause()
    }
}
