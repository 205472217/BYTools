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
    property var normalColor: undefined
    property var hoverColor: undefined
    property var pressColor: undefined
    property var borderColor: undefined
    property var defaultBorderColor: undefined
    property var disabledColor: undefined
    property var disabledBorderColor: undefined
    property var textColor: undefined
    property var hoverTextColor: undefined
    property var disabledTextColor: undefined
    property var shadowColor: undefined
    property bool showBorder: true

    readonly property var _p: themeManager.palette
    readonly property color _normalColor: root.normalColor !== undefined ? root.normalColor : (paletteGroup ? (_p[paletteGroup + "_normalColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _hoverColor: root.hoverColor !== undefined ? root.hoverColor : (paletteGroup ? (_p[paletteGroup + "_hoverColor"] || "#EEF4FF") : "#EEF4FF")
    readonly property color _pressColor: root.pressColor !== undefined ? root.pressColor : (paletteGroup ? (_p[paletteGroup + "_pressColor"] || "#DCE7FA") : "#DCE7FA")
    readonly property color _borderColor: root.borderColor !== undefined ? root.borderColor : (paletteGroup ? (_p[paletteGroup + "_borderColor"] || "#D8DEE9") : "#D8DEE9")
    readonly property color _defaultBorderColor: root.defaultBorderColor !== undefined ? root.defaultBorderColor : (paletteGroup ? (_p[paletteGroup + "_defaultBorderColor"] || "#BDBDBD") : "#BDBDBD")
    readonly property color _disabledColor: root.disabledColor !== undefined ? root.disabledColor : (paletteGroup ? (_p[paletteGroup + "_disabledColor"] || "#F1F5F9") : "#F1F5F9")
    readonly property color _disabledBorderColor: root.disabledBorderColor !== undefined ? root.disabledBorderColor : (paletteGroup ? (_p[paletteGroup + "_disabledBorderColor"] || "#E2E8F0") : "#E2E8F0")
    readonly property color _textColor: root.textColor !== undefined ? root.textColor : (paletteGroup ? (_p[paletteGroup + "_textColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _hoverTextColor: root.hoverTextColor !== undefined ? root.hoverTextColor : (paletteGroup ? (_p[paletteGroup + "_hoverTextColor"] || root._textColor) : root._textColor)
    readonly property color _disabledTextColor: root.disabledTextColor !== undefined ? root.disabledTextColor : (paletteGroup ? (_p[paletteGroup + "_disabledTextColor"] || "#94A3B8") : "#94A3B8")
    readonly property color _shadowColor: root.shadowColor !== undefined ? root.shadowColor : (paletteGroup ? (_p[paletteGroup + "_shadowColor"] || "#1E3A5F") : "#1E3A5F")

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
                color: {
                    if (!root.enabled) return root._disabledTextColor
                    if (mouseArea.pressed) return root._textColor
                    if (mouseArea.containsMouse) return root._hoverTextColor
                    return root._textColor
                }
                cached: true
            }
        }

        Label {
            id: labelText
            text: root.text
            visible: root.text.length > 0
            color: {
                if (!root.enabled) return root._disabledTextColor
                if (mouseArea.pressed) return root._textColor
                if (mouseArea.containsMouse) return root._hoverTextColor
                return root._textColor
            }
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
