//ComboBoxEx
import QtQuick
import QtQuick.Controls

ComboBox {
    id: root

    // ── 主题支持 ──
    property string paletteGroup: ""
    property color bgColor: "#FFFFFF"
    property color disabledBgColor: "#F8FAFC"
    property color textColor: "#1E293B"
    property color disabledTextColor: "#94A3B8"
    property color borderColor: "#BDBDBD"
    property color hoverBorderColor: "#CBD5E1"
    property color focusBorderColor: "#3B82F6"
    property color focusRingColor: "#3B82F6"
    property color arrowColor: "#64748B"
    property color disabledArrowColor: "#CBD5E1"
    property color popupBgColor: "#FFFFFF"
    property color popupBorderColor: "#BDBDBD"
    property color popupShadowColor: "#1E293B"
    property color delegateTextColor: "#334155"
    property color delegateHighlightTextColor: "#1E40AF"
    property color delegateHighlightBgColor: "#EFF6FF"

    readonly property var _p: themeManager.palette
    readonly property color _bgColor:
        paletteGroup ? (_p[paletteGroup + "_bgColor"] || bgColor) : bgColor
    readonly property color _disabledBgColor:
        paletteGroup ? (_p[paletteGroup + "_disabledBgColor"] || disabledBgColor) : disabledBgColor
    readonly property color _textColor:
        paletteGroup ? (_p[paletteGroup + "_textColor"] || textColor) : textColor
    readonly property color _disabledTextColor:
        paletteGroup ? (_p[paletteGroup + "_disabledTextColor"] || disabledTextColor) : disabledTextColor
    readonly property color _borderColor:
        paletteGroup ? (_p[paletteGroup + "_borderColor"] || borderColor) : borderColor
    readonly property color _hoverBorderColor:
        paletteGroup ? (_p[paletteGroup + "_hoverBorderColor"] || hoverBorderColor) : hoverBorderColor
    readonly property color _focusBorderColor:
        paletteGroup ? (_p[paletteGroup + "_focusBorderColor"] || focusBorderColor) : focusBorderColor
    readonly property color _focusRingColor:
        paletteGroup ? (_p[paletteGroup + "_focusRingColor"] || focusRingColor) : focusRingColor
    readonly property color _arrowColor:
        paletteGroup ? (_p[paletteGroup + "_arrowColor"] || arrowColor) : arrowColor
    readonly property color _disabledArrowColor:
        paletteGroup ? (_p[paletteGroup + "_disabledArrowColor"] || disabledArrowColor) : disabledArrowColor
    readonly property color _popupBgColor:
        paletteGroup ? (_p[paletteGroup + "_popupBgColor"] || popupBgColor) : popupBgColor
    readonly property color _popupBorderColor:
        paletteGroup ? (_p[paletteGroup + "_popupBorderColor"] || popupBorderColor) : popupBorderColor
    readonly property color _popupShadowColor:
        paletteGroup ? (_p[paletteGroup + "_popupShadowColor"] || popupShadowColor) : popupShadowColor
    readonly property color _delegateTextColor:
        paletteGroup ? (_p[paletteGroup + "_delegateTextColor"] || delegateTextColor) : delegateTextColor
    readonly property color _delegateHighlightTextColor:
        paletteGroup ? (_p[paletteGroup + "_delegateHighlightTextColor"] || delegateHighlightTextColor) : delegateHighlightTextColor
    readonly property color _delegateHighlightBgColor:
        paletteGroup ? (_p[paletteGroup + "_delegateHighlightBgColor"] || delegateHighlightBgColor) : delegateHighlightBgColor

    implicitWidth: 76
    implicitHeight: 26
    font.pixelSize: 13

    background: Rectangle {
        radius: 6
        color: root.enabled ? root._bgColor : root._disabledBgColor
        border.width: root.pressed || root.popup.visible ? 1.5 : 1
        border.color: root.pressed || root.popup.visible ? root._focusBorderColor :
                      root.hovered ? root._hoverBorderColor : root._borderColor

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
                color: root._focusRingColor
                opacity: 0.08
            }
        }


    }

    contentItem: Text {
        text: root.displayText
        font: root.font
        color: root.enabled ? root._textColor : root._disabledTextColor
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
            context.strokeStyle = root.enabled ? root._arrowColor : root._disabledArrowColor
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

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down)
            event.accepted = true
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
            color: root._popupBgColor
            border.color: root._popupBorderColor
            border.width: 1

            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                radius: 8
                color: root._popupShadowColor
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
            color: highlighted ? root._delegateHighlightTextColor : root._delegateTextColor
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }

        background: Rectangle {
            radius: 4
            color: highlighted ? root._delegateHighlightBgColor : "transparent"
        }

        highlighted: root.highlightedIndex === index
    }
}
