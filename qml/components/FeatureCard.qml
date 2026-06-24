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
    property color bgColor: "#fafbfc"
    property color hoverBgColor: "#ffffff"
    property color borderColor: "#e5e9f0"
    property color hoverBorderColor: "#3b82f6"
    property color accentColor: "#c7d2e0"
    property color hoverAccentColor: "#3b82f6"
    property color shadowColor: "#0d1b2a"
    property color iconGradientStart: "#6366f1"
    property color iconGradientEnd: "#8b5cf6"
    property color hoverIconGradientStart: "#3b82f6"
    property color hoverIconGradientEnd: "#2563eb"
    property color iconTextColor: "#ffffff"
    property color titleColor: "#172033"
    property color hoverTitleColor: "#1e40af"
    property color descriptionColor: "#627086"
    property color arrowColor: "#c7d2e0"
    property color hoverArrowColor: "#3b82f6"

    readonly property var _p: themeManager.palette
    readonly property color _bgColor:
        paletteGroup ? (_p[paletteGroup + "_bgColor"] || bgColor) : bgColor
    readonly property color _hoverBgColor:
        paletteGroup ? (_p[paletteGroup + "_hoverBgColor"] || hoverBgColor) : hoverBgColor
    readonly property color _borderColor:
        paletteGroup ? (_p[paletteGroup + "_borderColor"] || borderColor) : borderColor
    readonly property color _hoverBorderColor:
        paletteGroup ? (_p[paletteGroup + "_hoverBorderColor"] || hoverBorderColor) : hoverBorderColor
    readonly property color _accentColor:
        paletteGroup ? (_p[paletteGroup + "_accentColor"] || accentColor) : accentColor
    readonly property color _hoverAccentColor:
        paletteGroup ? (_p[paletteGroup + "_hoverAccentColor"] || hoverAccentColor) : hoverAccentColor
    readonly property color _shadowColor:
        paletteGroup ? (_p[paletteGroup + "_shadowColor"] || shadowColor) : shadowColor
    readonly property color _iconGradientStart:
        paletteGroup ? (_p[paletteGroup + "_iconGradientStart"] || iconGradientStart) : iconGradientStart
    readonly property color _iconGradientEnd:
        paletteGroup ? (_p[paletteGroup + "_iconGradientEnd"] || iconGradientEnd) : iconGradientEnd
    readonly property color _hoverIconGradientStart:
        paletteGroup ? (_p[paletteGroup + "_hoverIconGradientStart"] || hoverIconGradientStart) : hoverIconGradientStart
    readonly property color _hoverIconGradientEnd:
        paletteGroup ? (_p[paletteGroup + "_hoverIconGradientEnd"] || hoverIconGradientEnd) : hoverIconGradientEnd
    readonly property color _iconTextColor:
        paletteGroup ? (_p[paletteGroup + "_iconTextColor"] || iconTextColor) : iconTextColor
    readonly property color _titleColor:
        paletteGroup ? (_p[paletteGroup + "_titleColor"] || titleColor) : titleColor
    readonly property color _hoverTitleColor:
        paletteGroup ? (_p[paletteGroup + "_hoverTitleColor"] || hoverTitleColor) : hoverTitleColor
    readonly property color _descriptionColor:
        paletteGroup ? (_p[paletteGroup + "_descriptionColor"] || descriptionColor) : descriptionColor
    readonly property color _arrowColor:
        paletteGroup ? (_p[paletteGroup + "_arrowColor"] || arrowColor) : arrowColor
    readonly property color _hoverArrowColor:
        paletteGroup ? (_p[paletteGroup + "_hoverArrowColor"] || hoverArrowColor) : hoverArrowColor

    implicitHeight: 140

    signal clicked()

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
                GradientStop { position: 0.0; color: mouseArea.containsMouse ? root._hoverIconGradientStart : root._iconGradientStart }
                GradientStop { position: 1.0; color: mouseArea.containsMouse ? root._hoverIconGradientEnd : root._iconGradientEnd }
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
