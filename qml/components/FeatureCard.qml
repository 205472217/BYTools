import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string titleText: ""
    property string descriptionText: ""
    property string iconText: ""
    property string iconSource: ""

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

    implicitHeight: 140

    signal clicked()

    Rectangle {
        id: cardBg
        anchors.fill: parent
        radius: 12
        color: mouseArea.containsMouse ? root.hoverBgColor : root.bgColor
        border.color: mouseArea.containsMouse ? root.hoverBorderColor : root.borderColor
        border.width: 1

        Rectangle {
            width: 3
            height: parent.height - 24
            radius: 1.5
            color: mouseArea.containsMouse ? root.hoverAccentColor : root.accentColor
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter

            Behavior on color {
                ColorAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
        }

        Behavior on color {
            ColorAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "transparent"
        border.width: 0
        visible: mouseArea.containsMouse
        opacity: mouseArea.containsMouse ? 1 : 0

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 14
            color: root.shadowColor
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
                GradientStop { position: 0.0; color: mouseArea.containsMouse ? root.hoverIconGradientStart : root.iconGradientStart }
                GradientStop { position: 1.0; color: mouseArea.containsMouse ? root.hoverIconGradientEnd : root.iconGradientEnd }
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
                color: root.iconTextColor
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
                color: mouseArea.containsMouse ? root.hoverTitleColor : root.titleColor
                font.pixelSize: 18
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true

                Behavior on color {
                    ColorAnimation { duration: 200 }
                }
            }

            Label {
                text: root.descriptionText
                color: root.descriptionColor
                font.pixelSize: 13
                lineHeight: 1.35
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        Label {
            text: "\u203A"
            font.pixelSize: 24
            color: mouseArea.containsMouse ? root.hoverArrowColor : root.arrowColor
            opacity: mouseArea.containsMouse ? 1 : 0

            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
            Behavior on color {
                ColorAnimation { duration: 200 }
            }
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
