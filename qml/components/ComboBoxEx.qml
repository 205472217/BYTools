import QtQuick
import QtQuick.Controls

ComboBox {
    id: root

    property color borderColor: "#e2e8f0"
    property color focusBorderColor: "#3b82f6"

    implicitHeight: 36
    font.pixelSize: 13

    background: Rectangle {
        radius: 6
        color: root.enabled ? "#ffffff" : "#f8fafc"
        border.width: root.pressed || root.popup.visible ? 1.5 : 1
        border.color: root.pressed || root.popup.visible ? root.focusBorderColor :
                      root.hovered ? "#cbd5e1" : root.borderColor

        // 焦点/展开阴影
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
    }

    contentItem: Text {
        text: root.displayText
        font: root.font
        color: root.enabled ? "#1e293b" : "#94a3b8"
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
            context.strokeStyle = root.enabled ? "#64748b" : "#cbd5e1"
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
        Behavior on rotation {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
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
            color: "#ffffff"
            border.color: "#e2e8f0"
            border.width: 1

            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                radius: 8
                color: "#1e293b"
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

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 120 }
        }

        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 100 }
        }
    }

    delegate: ItemDelegate {
        width: root.width - 8
        height: 32

        contentItem: Text {
            text: modelData
            font.pixelSize: 13
            color: highlighted ? "#1e40af" : "#334155"
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }

        background: Rectangle {
            radius: 4
            color: highlighted ? "#eff6ff" : "transparent"
        }

        highlighted: root.highlightedIndex === index
    }
}
