//RadioButtonEx
import QtQuick
import QtQuick.Controls

RadioButton {
    id: root
    property color textColor: "#000000"

    implicitWidth: 76
    implicitHeight: 26

    contentItem: Text {
        id: radioText
        text: root.text
        color: root.textColor
        font: root.font
        verticalAlignment: Text.AlignVCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: root.indicator ? root.indicator.right : parent.left
        anchors.leftMargin: 8
        anchors.right: parent.right
    }
}
