import QtQuick
import QtQuick.Controls

TextField {
    id: root

    property color bgColor: "#ffffff"
    property color disabledBgColor: "#f8fafc"
    property color textColor: "#1e293b"
    property color disabledTextColor: "#94a3b8"
    property color phColor: "#b0bec5"
    property color selColor: "#3b82f6"
    property color selTextColor: "#ffffff"
    property color borderColor: "#BDBDBD"
    property color disabledBorderColor: "#e2e8f0"
    property color focusBorderColor: "#3b82f6"
    property color focusRingColor: "#3b82f6"
    property color cursorColor: "#3b82f6"

    implicitHeight: 26

    color: enabled ? root.textColor : root.disabledTextColor
    selectionColor: root.selColor
    selectedTextColor: root.selTextColor
    placeholderTextColor: root.phColor
    font.pixelSize: 13
    verticalAlignment: Text.AlignVCenter
    leftPadding: 12
    rightPadding: 12
    topPadding: 0
    bottomPadding: 0

    background: Rectangle {
        radius: 6
        color: root.enabled ? root.bgColor : root.disabledBgColor
        border.width: root.activeFocus ? 1.5 : 1
        border.color: root.activeFocus ? root.focusBorderColor :
                      root.enabled ? root.borderColor : root.disabledBorderColor

        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 8
            color: "transparent"
            border.width: 0
            visible: root.activeFocus && root.enabled

            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                radius: 10
                color: root.focusRingColor
                opacity: 0.08
            }
        }


    }

    cursorDelegate: Rectangle {
        width: 1.5
        color: root.cursorColor
        visible: root.activeFocus
    }
}
