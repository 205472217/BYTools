import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Pane {
    id: root

    signal openFeature(string featureId)

    padding: 0
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        color: "#f4f6f9"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 36
        spacing: 28

        // 头部区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: "功能集合"
                color: "#111827"
                font.pixelSize: 32
                font.bold: true
                font.letterSpacing: -0.5
            }

            Label {
                text: "选择一个工具开始处理任务"
                color: "#8492a6"
                font.pixelSize: 15
            }

            // 分隔线
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 12
                height: 1
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#3b82f6" }
                    GradientStop { position: 0.3; color: "#8b5cf6" }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 900 ? 3 : 2
            columnSpacing: 18
            rowSpacing: 18

            Repeater {
                model: appController.features

                FeatureCard {
                    Layout.fillWidth: true
                    titleText: modelData.name
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