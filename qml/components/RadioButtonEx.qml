//RadioButtonEx
import QtQuick
import QtQuick.Controls

RadioButton {
    id: root

    // ── 主题支持 ──
    property string paletteGroup: ""
    property var textColor: undefined

    readonly property var _p: themeManager.palette
    readonly property color _textColor: root.textColor !== undefined ? root.textColor : (paletteGroup ? (_p[paletteGroup + "_textColor"] || "#000000") : "#000000")

    implicitWidth: 76
    implicitHeight: 26

    contentItem: Text {
        id: radioText
        text: root.text
        color: root._textColor
        font: root.font
        verticalAlignment: Text.AlignVCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: root.indicator ? root.indicator.right : parent.left
        anchors.leftMargin: 8
        anchors.right: parent.right
    }
}
