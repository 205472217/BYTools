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
        audioOutput: AudioOutput {}
    }

    VideoOutput {
        anchors.fill: parent
        source: mediaPlayer
        fillMode: VideoOutput.PreserveAspectFit
    }
}
