import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string titleText: ""
    property string descriptionText: ""
    property string iconText: ""
    property string iconSource: ""

    implicitHeight: 132

    signal clicked()

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: mouseArea.containsMouse ? "#ffffff" : "#fbfcfe"
        border.color: mouseArea.containsMouse ? "#3b82f6" : "#d9dde5"
        border.width: mouseArea.containsMouse ? 2 : 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Rectangle {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            radius: 8
            color: "#e7f4f2"

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
                color: "#0f766e"
                font.pixelSize: 22
                font.bold: true
                visible: root.iconSource.length === 0
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: root.titleText
                color: "#172033"
                font.pixelSize: 18
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: root.descriptionText
                color: "#627086"
                font.pixelSize: 14
                lineHeight: 1.2
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: root.clicked()
        hoverEnabled: true
    }
}