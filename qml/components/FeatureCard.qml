import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string titleText: ""
    property string descriptionText: ""
    property string iconText: ""
    property string iconSource: ""

    implicitHeight: 140

    signal clicked()

    Rectangle {
        id: cardBg
        anchors.fill: parent
        radius: 12
        color: mouseArea.containsMouse ? "#ffffff" : "#fafbfc"
        border.color: mouseArea.containsMouse ? "#3b82f6" : "#e5e9f0"
        border.width: 1

        // 左侧彩色装饰条
        Rectangle {
            width: 3
            height: parent.height - 24
            radius: 1.5
            color: mouseArea.containsMouse ? "#3b82f6" : "#c7d2e0"
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

    // 悬浮阴影
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

        // 使用矩形模拟阴影效果
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 14
            color: "#0d1b2a"
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
                GradientStop { position: 0.0; color: mouseArea.containsMouse ? "#3b82f6" : "#6366f1" }
                GradientStop { position: 1.0; color: mouseArea.containsMouse ? "#2563eb" : "#8b5cf6" }
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
                color: "#ffffff"
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
                color: mouseArea.containsMouse ? "#1e40af" : "#172033"
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
                color: "#627086"
                font.pixelSize: 13
                lineHeight: 1.35
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        // 箭头指示器
        Label {
            text: "\u203A"
            font.pixelSize: 24
            color: mouseArea.containsMouse ? "#3b82f6" : "#c7d2e0"
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