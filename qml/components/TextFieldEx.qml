import QtQuick
import QtQuick.Controls

TextField {
    id: root

    property color borderColor: "#e2e8f0"
    property color focusBorderColor: "#3b82f6"
    property color disabledBorderColor: "#f1f5f9"
    property color disabledBgColor: "#f8fafc"

    implicitHeight: 36

    color: enabled ? "#1e293b" : "#94a3b8"
    selectionColor: "#3b82f6"
    selectedTextColor: "#ffffff"
    placeholderTextColor: "#b0bec5"
    font.pixelSize: 13
    verticalAlignment: Text.AlignVCenter
    leftPadding: 12
    rightPadding: 12
    topPadding: 0
    bottomPadding: 0

    background: Rectangle {
        radius: 6
        color: root.enabled ? "#ffffff" : root.disabledBgColor
        border.width: root.activeFocus ? 1.5 : 1
        border.color: root.activeFocus ? root.focusBorderColor :
                      root.enabled ? root.borderColor : root.disabledBorderColor

        // 焦点阴影
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
                color: "#3b82f6"
                opacity: 0.08
            }
        }

        Behavior on border.color {
            ColorAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
        Behavior on border.width {
            NumberAnimation { duration: 100 }
        }
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    cursorDelegate: Rectangle {
        width: 1.5
        color: "#3b82f6"
        visible: root.activeFocus
    }
}
