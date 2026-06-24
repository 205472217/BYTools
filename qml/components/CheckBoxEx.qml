//CheckBoxEx
import QtQuick
import QtQuick.Controls

CheckBox {
    id: root

    // ── 主题支持 ──
    property string paletteGroup: ""
    property color textColor: "#000000"

    readonly property var _p: themeManager.palette
    readonly property color _textColor:
        paletteGroup ? (_p[paletteGroup + "_textColor"] || textColor) : textColor

    implicitWidth: 76
    implicitHeight: 26

    contentItem: Text {
        id: checkText
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