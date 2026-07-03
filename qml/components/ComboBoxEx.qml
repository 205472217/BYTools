//ComboBoxEx
import QtQuick
import QtQuick.Controls

ComboBox {
    id: root

    // ── 主题支持 ──
    property string paletteGroup: ""
    property var bgColor: undefined
    property var disabledBgColor: undefined
    property var textColor: undefined
    property var disabledTextColor: undefined
    property var borderColor: undefined
    property var hoverBorderColor: undefined
    property var focusBorderColor: undefined
    property var focusRingColor: undefined
    property var arrowColor: undefined
    property var disabledArrowColor: undefined
    property var popupBgColor: undefined
    property var popupBorderColor: undefined
    property var popupShadowColor: undefined
    property var delegateTextColor: undefined
    property var delegateHighlightTextColor: undefined
    property var delegateHighlightBgColor: undefined

    readonly property var _p: themeManager.palette
    readonly property color _bgColor: root.bgColor !== undefined ? root.bgColor : (paletteGroup ? (_p[paletteGroup + "_bgColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _disabledBgColor: root.disabledBgColor !== undefined ? root.disabledBgColor : (paletteGroup ? (_p[paletteGroup + "_disabledBgColor"] || "#F8FAFC") : "#F8FAFC")
    readonly property color _textColor: root.textColor !== undefined ? root.textColor : (paletteGroup ? (_p[paletteGroup + "_textColor"] || "#1E293B") : "#1E293B")
    readonly property color _disabledTextColor: root.disabledTextColor !== undefined ? root.disabledTextColor : (paletteGroup ? (_p[paletteGroup + "_disabledTextColor"] || "#94A3B8") : "#94A3B8")
    readonly property color _borderColor: root.borderColor !== undefined ? root.borderColor : (paletteGroup ? (_p[paletteGroup + "_borderColor"] || "#BDBDBD") : "#BDBDBD")
    readonly property color _hoverBorderColor: root.hoverBorderColor !== undefined ? root.hoverBorderColor : (paletteGroup ? (_p[paletteGroup + "_hoverBorderColor"] || "#CBD5E1") : "#CBD5E1")
    readonly property color _focusBorderColor: root.focusBorderColor !== undefined ? root.focusBorderColor : (paletteGroup ? (_p[paletteGroup + "_focusBorderColor"] || "#3B82F6") : "#3B82F6")
    readonly property color _focusRingColor: root.focusRingColor !== undefined ? root.focusRingColor : (paletteGroup ? (_p[paletteGroup + "_focusRingColor"] || "#3B82F6") : "#3B82F6")
    readonly property color _arrowColor: root.arrowColor !== undefined ? root.arrowColor : (paletteGroup ? (_p[paletteGroup + "_arrowColor"] || "#64748B") : "#64748B")
    readonly property color _disabledArrowColor: root.disabledArrowColor !== undefined ? root.disabledArrowColor : (paletteGroup ? (_p[paletteGroup + "_disabledArrowColor"] || "#CBD5E1") : "#CBD5E1")
    readonly property color _popupBgColor: root.popupBgColor !== undefined ? root.popupBgColor : (paletteGroup ? (_p[paletteGroup + "_popupBgColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _popupBorderColor: root.popupBorderColor !== undefined ? root.popupBorderColor : (paletteGroup ? (_p[paletteGroup + "_popupBorderColor"] || "#BDBDBD") : "#BDBDBD")
    readonly property color _popupShadowColor: root.popupShadowColor !== undefined ? root.popupShadowColor : (paletteGroup ? (_p[paletteGroup + "_popupShadowColor"] || "#1E293B") : "#1E293B")
    readonly property color _delegateTextColor: root.delegateTextColor !== undefined ? root.delegateTextColor : (paletteGroup ? (_p[paletteGroup + "_delegateTextColor"] || "#334155") : "#334155")
    readonly property color _delegateHighlightTextColor: root.delegateHighlightTextColor !== undefined ? root.delegateHighlightTextColor : (paletteGroup ? (_p[paletteGroup + "_delegateHighlightTextColor"] || "#1E40AF") : "#1E40AF")
    readonly property color _delegateHighlightBgColor: root.delegateHighlightBgColor !== undefined ? root.delegateHighlightBgColor : (paletteGroup ? (_p[paletteGroup + "_delegateHighlightBgColor"] || "#EFF6FF") : "#EFF6FF")

    implicitWidth: 76
    implicitHeight: 26
    font.pixelSize: 13

    background: Rectangle {
        radius: 6
        color: root.enabled ? root._bgColor : root._disabledBgColor
        border.width: root.pressed || root.popup.visible ? 1.5 : 1
        border.color: root.pressed || root.popup.visible ? root._focusBorderColor : root.hovered ? root._hoverBorderColor : root._borderColor

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
            context.reset();
            context.strokeStyle = root.enabled ? root._arrowColor : root._disabledArrowColor;
            context.lineWidth = 1.5;
            context.lineCap = "round";
            context.lineJoin = "round";
            context.beginPath();
            context.moveTo(1, 2);
            context.lineTo(6, 6);
            context.lineTo(11, 2);
            context.stroke();
        }

        onRotationChanged: requestPaint()

        rotation: root.popup.visible ? 180 : 0
    }

    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down)
            event.accepted = true;
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
