//ImageViewer
import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var pal: themeManager.palette
    property string source: ""
    property bool hasPrevious: false
    property bool hasNext: false

    signal previousRequested()
    signal nextRequested()
    signal deleteRequested()

    Shortcut {
        sequence: "Left"
        enabled: root.visible && root.hasPrevious
        onActivated: root.previousRequested()
    }
    Shortcut {
        sequence: "Up"
        enabled: root.visible && root.hasPrevious
        onActivated: root.previousRequested()
    }
    Shortcut {
        sequence: "Right"
        enabled: root.visible && root.hasNext
        onActivated: root.nextRequested()
    }
    Shortcut {
        sequence: "Down"
        enabled: root.visible && root.hasNext
        onActivated: root.nextRequested()
    }
    Shortcut {
        sequence: "Delete"
        enabled: root.visible
        onActivated: root.deleteRequested()
    }

    Image {
        id: previewImage
        anchors.fill: parent
        source: root.source.length > 0 ? "file:///" + root.source : ""
        fillMode: Image.PreserveAspectFit
        autoTransform: true
        asynchronous: true
        smooth: true
    }

    // ── Previous button ──
    Rectangle {
        id: prevBtn
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 4
        width: 36
        height: 60
        radius: 10
        color: prevBtnMouse.containsMouse ? pal.IconBtnEx_overlayBgHoverColor : pal.IconBtnEx_overlayBgColor
        visible: root.source.length > 0 && root.hasPrevious
        opacity: prevBtnMouse.containsMouse ? 1.0 : 0.55

        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }

        Label {
            anchors.centerIn: parent
            text: "‹"
            color: prevBtnMouse.containsMouse ? pal.IconBtnEx_overlayTextHoverColor : pal.IconBtnEx_overlayTextColor
            font.pixelSize: 26
            font.bold: true
        }

        MouseArea {
            id: prevBtnMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.previousRequested()
        }
    }

    // ── Next button ──
    Rectangle {
        id: nextBtn
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 4
        width: 36
        height: 60
        radius: 10
        color: nextBtnMouse.containsMouse ? pal.IconBtnEx_overlayBgHoverColor : pal.IconBtnEx_overlayBgColor
        visible: root.source.length > 0 && root.hasNext
        opacity: nextBtnMouse.containsMouse ? 1.0 : 0.55

        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }

        Label {
            anchors.centerIn: parent
            text: "›"
            color: nextBtnMouse.containsMouse ? pal.IconBtnEx_overlayTextHoverColor : pal.IconBtnEx_overlayTextColor
            font.pixelSize: 26
            font.bold: true
        }

        MouseArea {
            id: nextBtnMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.nextRequested()
        }
    }
}
