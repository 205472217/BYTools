import QtQuick
import QtQuick.Controls

ComboBox {
    id: root

    property color bgColor: "#ffffff"
    property color disabledBgColor: "#f8fafc"
    property color textColor: "#1e293b"
    property color disabledTextColor: "#94a3b8"
    property color borderColor: "#BDBDBD"
    property color hoverBorderColor: "#cbd5e1"
    property color focusBorderColor: "#3b82f6"
    property color focusRingColor: "#3b82f6"
    property color arrowColor: "#64748b"
    property color disabledArrowColor: "#cbd5e1"
    property color popupBgColor: "#ffffff"
    property color popupBorderColor: "#BDBDBD"
    property color popupShadowColor: "#1e293b"
    property color delegateTextColor: "#334155"
    property color delegateHighlightTextColor: "#1e40af"
    property color delegateHighlightBgColor: "#eff6ff"

    implicitWidth: 76
    implicitHeight: 26
    font.pixelSize: 13

    background: Rectangle {
        radius: 6
        color: root.enabled ? root.bgColor : root.disabledBgColor
        border.width: root.pressed || root.popup.visible ? 1.5 : 1
        border.color: root.pressed || root.popup.visible ? root.focusBorderColor :
                      root.hovered ? root.hoverBorderColor : root.borderColor

        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 8
            color: "transparent"
            border.width: 0
            visible: root.pressed || root.popup.visible

            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                radius: 10
                color: root.focusRingColor
                opacity: 0.08
            }
        }


    }

    contentItem: Text {
        text: root.displayText
        font: root.font
        color: root.enabled ? root.textColor : root.disabledTextColor
        verticalAlignment: Text.AlignVCenter
        leftPadding: 12
        rightPadding: root.indicator.width + 8
        elide: Text.ElideRight
    }

    indicator: Canvas {
        x: root.width - width - 10
        y: root.topPadding + (root.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        onPaint: {
            context.reset()
            context.strokeStyle = root.enabled ? root.arrowColor : root.disabledArrowColor
            context.lineWidth = 1.5
            context.lineCap = "round"
            context.lineJoin = "round"
            context.beginPath()
            context.moveTo(1, 2)
            context.lineTo(6, 6)
            context.lineTo(11, 2)
            context.stroke()
        }

        onRotationChanged: requestPaint()

        rotation: root.popup.visible ? 180 : 0

    }

    popup: Popup {
        y: root.height
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, 200)
        padding: 4
        topPadding: 4
        bottomPadding: 4

        background: Rectangle {
            radius: 6
            color: root.popupBgColor
            border.color: root.popupBorderColor
            border.width: 1

            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                radius: 8
                color: root.popupShadowColor
                opacity: 0.08
                z: -1
            }
        }

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.delegateModel
            currentIndex: root.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator {}
        }


    }

    delegate: ItemDelegate {
        width: root.width - 8
        height: 32

        contentItem: Text {
            text: root.textRole ? modelData[root.textRole] : modelData
            font.pixelSize: 13
            color: highlighted ? root.delegateHighlightTextColor : root.delegateTextColor
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }

        background: Rectangle {
            radius: 4
            color: highlighted ? root.delegateHighlightBgColor : "transparent"
        }

        highlighted: root.highlightedIndex === index
    }
}
