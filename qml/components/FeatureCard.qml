import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    property string titleText: ""
    property string descriptionText: ""
    property string iconText: ""
    property string iconSource: ""

    implicitHeight: 132
    padding: 0
    hoverEnabled: true

    background: Rectangle {
        radius: 8
        color: root.hovered ? "#ffffff" : "#fbfcfe"
        border.color: root.hovered ? "#3b82f6" : "#d9dde5"
        border.width: root.hovered ? 2 : 1
    }

    contentItem: RowLayout {
        spacing: 16
        anchors.fill: parent
        anchors.margins: 18

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
}
