import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string iconSource: ""
    property string text: ""
    property color textColor: "#ffffff"
    property string tooltip: text
    property color normalColor: "#ffffff"
    property color hoverColor: "#eef4ff"
    property color pressColor: "#dce7fa"
    property color borderColor: "#d8dee9"
    property bool enabled: true

    implicitWidth: text.length > 0 ? 76 : 38
    implicitHeight: 38

    signal clicked()

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 8
        color: root.enabled ? (mouseArea.pressed ? root.pressColor :
               mouseArea.containsMouse ? root.hoverColor : root.normalColor) : "#f1f5f9"
        border.width: 1
        border.color: root.enabled ? (mouseArea.pressed ? root.borderColor :
                      mouseArea.containsMouse ? root.borderColor : "#e2e8f0") : "#e2e8f0"

        // 悬浮时的微妙阴影
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: 9
            color: "transparent"
            border.width: 0
            z: -1
            visible: mouseArea.containsMouse && root.enabled

            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                radius: 10
                color: "#1e3a5f"
                opacity: 0.06
            }
        }

        Behavior on color {
            ColorAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 6
        scale: mouseArea.pressed ? 0.9 : 1.0
        Behavior on scale {
            NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
        }

        Image {
            id: iconImg
            width: 18
            height: 18
            source: root.iconSource
            visible: root.iconSource.length > 0
            anchors.verticalCenter: parent.verticalCenter
            opacity: root.enabled ? 1.0 : 0.4
        }

        Label {
            id: labelText
            text: root.text
            visible: root.text.length > 0
            color: root.enabled ? root.textColor : "#94a3b8"
            font.pixelSize: 13
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled
        onClicked: root.clicked()
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    ToolTip.visible: mouseArea.containsMouse && root.tooltip.length > 0
    ToolTip.delay: 500
    ToolTip.text: root.tooltip
}