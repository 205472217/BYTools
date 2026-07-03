//FeatureCard
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string titleText: ""
    property string descriptionText: ""
    property string iconText: ""
    property string iconSource: ""

    // ── 主题支持 ──
    property string paletteGroup: ""
    property var bgColor: undefined
    property var hoverBgColor: undefined
    property var borderColor: undefined
    property var hoverBorderColor: undefined
    property var accentColor: undefined
    property var hoverAccentColor: undefined
    property var shadowColor: undefined
    property var iconGradientStart: undefined
    property var iconGradientEnd: undefined
    property var hoverIconGradientStart: undefined
    property var hoverIconGradientEnd: undefined
    property var iconTextColor: undefined
    property var titleColor: undefined
    property var hoverTitleColor: undefined
    property var descriptionColor: undefined
    property var arrowColor: undefined
    property var hoverArrowColor: undefined

    readonly property var _p: themeManager.palette
    readonly property color _bgColor: root.bgColor !== undefined ? root.bgColor : (paletteGroup ? (_p[paletteGroup + "_bgColor"] || "#FAFBFC") : "#FAFBFC")
    readonly property color _hoverBgColor: root.hoverBgColor !== undefined ? root.hoverBgColor : (paletteGroup ? (_p[paletteGroup + "_hoverBgColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _borderColor: root.borderColor !== undefined ? root.borderColor : (paletteGroup ? (_p[paletteGroup + "_borderColor"] || "#E5E9F0") : "#E5E9F0")
    readonly property color _hoverBorderColor: root.hoverBorderColor !== undefined ? root.hoverBorderColor : (paletteGroup ? (_p[paletteGroup + "_hoverBorderColor"] || "#3B82F6") : "#3B82F6")
    readonly property color _accentColor: root.accentColor !== undefined ? root.accentColor : (paletteGroup ? (_p[paletteGroup + "_accentColor"] || "#C7D2E0") : "#C7D2E0")
    readonly property color _hoverAccentColor: root.hoverAccentColor !== undefined ? root.hoverAccentColor : (paletteGroup ? (_p[paletteGroup + "_hoverAccentColor"] || "#3B82F6") : "#3B82F6")
    readonly property color _shadowColor: root.shadowColor !== undefined ? root.shadowColor : (paletteGroup ? (_p[paletteGroup + "_shadowColor"] || "#0D1B2A") : "#0D1B2A")
    readonly property color _iconGradientStart: root.iconGradientStart !== undefined ? root.iconGradientStart : (paletteGroup ? (_p[paletteGroup + "_iconGradientStart"] || "#6366F1") : "#6366F1")
    readonly property color _iconGradientEnd: root.iconGradientEnd !== undefined ? root.iconGradientEnd : (paletteGroup ? (_p[paletteGroup + "_iconGradientEnd"] || "#8B5CF6") : "#8B5CF6")
    readonly property color _hoverIconGradientStart: root.hoverIconGradientStart !== undefined ? root.hoverIconGradientStart : (paletteGroup ? (_p[paletteGroup + "_hoverIconGradientStart"] || "#3B82F6") : "#3B82F6")
    readonly property color _hoverIconGradientEnd: root.hoverIconGradientEnd !== undefined ? root.hoverIconGradientEnd : (paletteGroup ? (_p[paletteGroup + "_hoverIconGradientEnd"] || "#2563EB") : "#2563EB")
    readonly property color _iconTextColor: root.iconTextColor !== undefined ? root.iconTextColor : (paletteGroup ? (_p[paletteGroup + "_iconTextColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _titleColor: root.titleColor !== undefined ? root.titleColor : (paletteGroup ? (_p[paletteGroup + "_titleColor"] || "#172033") : "#172033")
    readonly property color _hoverTitleColor: root.hoverTitleColor !== undefined ? root.hoverTitleColor : (paletteGroup ? (_p[paletteGroup + "_hoverTitleColor"] || "#1E40AF") : "#1E40AF")
    readonly property color _descriptionColor: root.descriptionColor !== undefined ? root.descriptionColor : (paletteGroup ? (_p[paletteGroup + "_descriptionColor"] || "#627086") : "#627086")
    readonly property color _arrowColor: root.arrowColor !== undefined ? root.arrowColor : (paletteGroup ? (_p[paletteGroup + "_arrowColor"] || "#C7D2E0") : "#C7D2E0")
    readonly property color _hoverArrowColor: root.hoverArrowColor !== undefined ? root.hoverArrowColor : (paletteGroup ? (_p[paletteGroup + "_hoverArrowColor"] || "#3B82F6") : "#3B82F6")

    implicitHeight: 140

    signal clicked

    Rectangle {
        id: cardBg
        anchors.fill: parent
        radius: 12
        color: mouseArea.containsMouse ? root._hoverBgColor : root._bgColor
        border.color: mouseArea.containsMouse ? root._hoverBorderColor : root._borderColor
        border.width: 1

        Rectangle {
            width: 3
            height: parent.height - 24
            radius: 1.5
            color: mouseArea.containsMouse ? root._hoverAccentColor : root._accentColor
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "transparent"
        border.width: 0
        visible: mouseArea.containsMouse
        opacity: mouseArea.containsMouse ? 1 : 0

        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 14
            color: root._shadowColor
            opacity: 0.06
            z: -1
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 22
        anchors.rightMargin: 18
        anchors.topMargin: 20
        anchors.bottomMargin: 20
        spacing: 16

        Rectangle {
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50
            radius: 12
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: mouseArea.containsMouse ? root._hoverIconGradientStart : root._iconGradientStart
                }
                GradientStop {
                    position: 1.0
                    color: mouseArea.containsMouse ? root._hoverIconGradientEnd : root._iconGradientEnd
                }
            }

            Image {
                anchors.centerIn: parent
                width: 24
                height: 24
                source: root.iconSource
                fillMode: Image.PreserveAspectFit
                visible: root.iconSource.length > 0
            }

            Text {
                anchors.centerIn: parent
                text: root.iconText
                color: root._iconTextColor
                font.pixelSize: 22
                font.bold: true
                visible: root.iconSource.length === 0
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
                text: root.titleText
                color: mouseArea.containsMouse ? root._hoverTitleColor : root._titleColor
                font.pixelSize: 18
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: root.descriptionText
                color: root._descriptionColor
                font.pixelSize: 13
                lineHeight: 1.35
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        Label {
            text: "\u203A"
            font.pixelSize: 24
            color: mouseArea.containsMouse ? root._hoverArrowColor : root._arrowColor
            opacity: mouseArea.containsMouse ? 1 : 0
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: root.clicked()
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
    }
}
