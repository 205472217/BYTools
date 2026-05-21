import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string iconSource: ""
    property string tooltip: text
    property color normalColor: "#ffffff"
    property color hoverColor: "#eef4ff"
    property color borderColor: "#d8dee9"
    property color disabledColor: "#f1f5f9"
    property bool enabled: true

    implicitWidth: 38
    implicitHeight: 38

    signal clicked()

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: !root.enabled ? root.disabledColor : mouseArea.containsMouse ? root.hoverColor : root.normalColor
        border.width: 1
        border.color: root.borderColor
    }

    Image {
        anchors.centerIn: parent
        width: 18
        height: 18
        source: root.iconSource
        visible: root.enabled
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled
        onClicked: root.clicked()
        hoverEnabled: true
    }

    ToolTip.visible: mouseArea.containsMouse && root.tooltip.length > 0
    ToolTip.delay: 400
    ToolTip.text: root.tooltip
}