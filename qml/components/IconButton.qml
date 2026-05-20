import QtQuick
import QtQuick.Controls

Button {
    id: root

    property string iconSource: ""
    property string tooltip: text
    property color normalColor: "#ffffff"
    property color hoverColor: "#eef4ff"
    property color borderColor: "#d8dee9"
    property color disabledColor: "#f1f5f9"

    implicitWidth: 38
    implicitHeight: 38
    padding: 0
    display: AbstractButton.IconOnly
    hoverEnabled: true

    icon.source: iconSource
    icon.width: 18
    icon.height: 18

    background: Rectangle {
        radius: 8
        color: !root.enabled ? root.disabledColor : root.hovered ? root.hoverColor : root.normalColor
        border.color: root.enabled ? root.borderColor : "#e2e8f0"
    }

    ToolTip.visible: root.hovered && root.tooltip.length > 0
    ToolTip.delay: 400
    ToolTip.text: root.tooltip
}
