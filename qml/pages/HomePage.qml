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

    property bool showAll: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 36
        spacing: 20

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

        // 显示模式切换
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            Label {
                text: "显示模式:"
                color: "#627086"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignVCenter
            }

            RadioButton {
                id: showAllRadio
                text: "全部插件"
                checked: true
                onCheckedChanged: { if (checked) root.showAll = true; }
            }

            RadioButton {
                id: categoryRadio
                text: "分类显示"
                onCheckedChanged: { if (checked) root.showAll = false; }
            }
        }

        // ======== 全部插件视图 ========
        Flickable {
            id: pluginFlick
            visible: root.showAll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: gridLayout.implicitWidth
            contentHeight: gridLayout.implicitHeight
            flickableDirection: Flickable.VerticalFlick
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
                interactive: true
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }

            GridLayout {
                id: gridLayout
                width: pluginFlick.width - 12
                columns: 2
                columnSpacing: 18
                rowSpacing: 18

                Repeater {
                    model: appController ? appController.features : []

                    FeatureCard {
                        Layout.fillWidth: true
                        titleText: modelData.name
                        descriptionText: modelData.description
                        iconSource: "qrc:/icons/card-" + modelData.id + ".svg"
                        onClicked: root.openFeature(modelData.id)
                    }
                }
            }
        }

        // ======== 分类视图 ========
        ColumnLayout {
            visible: !root.showAll
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            TabBar {
                id: tabBar
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                spacing: 4
                clip: true
                background: Rectangle {
                    color: "#ffffff"
                    radius: 8
                    border.color: "#e5e9f0"
                    border.width: 1
                }

                Repeater {
                    model: appController ? appController.pluginCategories : []
                    delegate: TabButton {
                        id: tabBtn
                        text: modelData
                        font.pixelSize: 13
                        font.bold: tabBar.currentIndex === index
                        padding: 13

                        contentItem: Text {
                            text: tabBtn.text
                            color: tabBar.currentIndex === index ? "#ffffff" : "#627086"
                            font: tabBtn.font
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: tabBar.currentIndex === index ? "#3b82f6" : tabBtn.hovered ? "#f0f4ff" : "transparent"
                            radius: 6
                            anchors.fill: parent
                            anchors.margins: 3
                        }
                    }
                }
            }

            Flickable {
                id: categoryFlick
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: categoryGrid.implicitWidth
                contentHeight: categoryGrid.implicitHeight
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                    interactive: true
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                }

                GridLayout {
                    id: categoryGrid
                    width: categoryFlick.width - 12
                    columns: 2
                    columnSpacing: 18
                    rowSpacing: 18

                    Repeater {
                        model: appController ? appController.features.filter(function(f) {
                            return appController && f.category === appController.pluginCategories[tabBar.currentIndex]
                        }) : []

                        FeatureCard {
                            Layout.fillWidth: true
                            titleText: modelData.name
                            descriptionText: modelData.description
                            iconSource: "qrc:/icons/card-" + modelData.id + ".svg"
                            onClicked: root.openFeature(modelData.id)
                        }
                    }
                }
            }
        }
    }
}