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
    property color defaultBorderColor: "#e2e8f0"
    property color disabledColor: "#f1f5f9"
    property color disabledBorderColor: "#e2e8f0"
    property color disabledTextColor: "#94a3b8"
    property color shadowColor: "#1e3a5f"
    property bool enabled: true

    implicitWidth: text.length > 0 ? 76 : 38
    implicitHeight: 38

    signal clicked()

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 8
        color: root.enabled ? (mouseArea.pressed ? root.pressColor :
               mouseArea.containsMouse ? root.hoverColor : root.normalColor) : root.disabledColor
        border.width: 1
        border.color: root.enabled ? (mouseArea.pressed ? root.borderColor :
                      mouseArea.containsMouse ? root.borderColor : root.defaultBorderColor) : root.disabledBorderColor

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
                color: root.shadowColor
                opacity: 0.06
            }
        }


    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 6
        scale: mouseArea.pressed ? 0.9 : 1.0


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
            color: root.enabled ? root.textColor : root.disabledTextColor
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
