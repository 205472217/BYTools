import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Pane {
    id: root

    signal openFeature(string featureId)

    padding: 0
    background: Rectangle {
        color: "#f6f7f9"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 24

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Label {
                    text: "功能集合"
                    color: "#111827"
                    font.pixelSize: 30
                    font.bold: true
                }

                Label {
                    text: "选择一个工具开始处理任务"
                    color: "#64748b"
                    font.pixelSize: 15
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 900 ? 3 : 2
            columnSpacing: 16
            rowSpacing: 16

            Repeater {
                model: appController.features

                FeatureCard {
                    Layout.fillWidth: true
                    titleText: modelData.title
                    descriptionText: modelData.description
                    iconText: "文"
                    iconSource: "../icons/languages.svg"
                    onClicked: root.openFeature(modelData.id)
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
