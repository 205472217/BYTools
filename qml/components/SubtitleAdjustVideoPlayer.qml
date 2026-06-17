import QtQuick
import QtMultimedia

Item {
    id: root

    property alias source: mediaPlayer.source
    property alias position: mediaPlayer.position
    property alias duration: mediaPlayer.duration
    property alias playbackState: mediaPlayer.playbackState

    function play() { mediaPlayer.play() }
    function pause() { mediaPlayer.pause() }
    function stop() { mediaPlayer.stop() }

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
        fillMode: VideoOutput.PreserveAspectFit
    }

    onVisibleChanged: {
        if (!root.visible && mediaPlayer.playbackState === 1)
            mediaPlayer.pause()
    }
}
