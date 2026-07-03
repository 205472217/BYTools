//IconButton
import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

Item {
    id: root

    property string iconSource: ""
    property string text: ""
    property string tooltip: text
    property bool enabled: true

    // ── 主题支持 ──
    property string paletteGroup: ""
    property color normalColor: "#FFFFFF"
    property color hoverColor: "#EEF4FF"
    property color pressColor: "#DCE7FA"
    property color borderColor: "#D8DEE9"
    property color defaultBorderColor: "#BDBDBD"
    property color disabledColor: "#F1F5F9"
    property color disabledBorderColor: "#E2E8F0"
    property color textColor: "#FFFFFF"
    property color disabledTextColor: "#94A3B8"
    property color shadowColor: "#1E3A5F"
    property bool showBorder: true

    readonly property var _p: themeManager.palette
    readonly property color _normalColor: paletteGroup ? (_p[paletteGroup + "_normalColor"] || normalColor) : normalColor
    readonly property color _hoverColor: paletteGroup ? (_p[paletteGroup + "_hoverColor"] || hoverColor) : hoverColor
    readonly property color _pressColor: paletteGroup ? (_p[paletteGroup + "_pressColor"] || pressColor) : pressColor
    readonly property color _borderColor: paletteGroup ? (_p[paletteGroup + "_borderColor"] || borderColor) : borderColor
    readonly property color _defaultBorderColor: paletteGroup ? (_p[paletteGroup + "_defaultBorderColor"] || defaultBorderColor) : defaultBorderColor
    readonly property color _disabledColor: paletteGroup ? (_p[paletteGroup + "_disabledColor"] || disabledColor) : disabledColor
    readonly property color _disabledBorderColor: paletteGroup ? (_p[paletteGroup + "_disabledBorderColor"] || disabledBorderColor) : disabledBorderColor
    readonly property color _textColor: paletteGroup ? (_p[paletteGroup + "_textColor"] || textColor) : textColor
    readonly property color _disabledTextColor: paletteGroup ? (_p[paletteGroup + "_disabledTextColor"] || disabledTextColor) : disabledTextColor
    readonly property color _shadowColor: paletteGroup ? (_p[paletteGroup + "_shadowColor"] || shadowColor) : shadowColor

    implicitWidth: text.length > 0 ? 76 : 38
    implicitHeight: 26

    signal clicked

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 8
        color: root.enabled ? (mouseArea.pressed ? root._pressColor : mouseArea.containsMouse ? root._hoverColor : root._normalColor) : root._disabledColor
        border.width: root.showBorder ? 1 : 0
        border.color: root.enabled ? (mouseArea.pressed ? root._borderColor : mouseArea.containsMouse ? root._borderColor : root._defaultBorderColor) : root._disabledBorderColor

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
                color: root._shadowColor
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
            layer.enabled: true
            layer.samplerName: "source"
            layer.effect: ColorOverlay {
                color: root.enabled ? root._textColor : root._disabledTextColor
                cached: true
            }
        }

        Label {
            id: labelText
            text: root.text
            visible: root.text.length > 0
            color: root.enabled ? root._textColor : root._disabledTextColor
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
