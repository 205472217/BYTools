import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Pane {
    id: root

    signal openFeature(string featureId)

    padding: 0
    focusPolicy: Qt.NoFocus

    property var pal: themeManager.palette
    property bool showAll: true

    background: Rectangle {
        id: bgRect
        color: pal.HomePage_bgRect_color
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 36
        spacing: 20

        // 头部区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    id: titleLabel
                    text: "功能集合"
                    color: pal.HomePage_titleLabel_color
                    font.pixelSize: 32
                    font.bold: true
                    font.letterSpacing: -0.5
                }

                Label {
                    id: subtitleLabel
                    text: "选择一个工具开始处理任务"
                    color: pal.HomePage_subtitleLabel_color
                    font.pixelSize: 15
                }

                // 分隔线
                Rectangle {
                    id: headerSeparator
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    height: 1
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: pal.HomePage_headerSepStart_color }
                        GradientStop { position: 0.3; color: pal.HomePage_headerSepMid_color }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }
            }

            // 主题切换
            TabBar {
                id: themeSwitcher
                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: 30
                spacing: 0
                clip: true

                background: Rectangle {
                    color: pal.HomePage_themeSwitcherBg_color
                    radius: 6
                    border.color: pal.HomePage_themeSwitcherBorder_color
                    border.width: 1
                }

                TabButton {
                    id: lightTab
                    width: 60
                    text: "Light"
                    font.pixelSize: 12
                    font.bold: themeSwitcher.currentIndex === 0
                    padding: 8

                    contentItem: Text {
                        text: lightTab.text
                        color: themeSwitcher.currentIndex === 0 ? pal.HomePage_themeTabLabel_color_active : pal.HomePage_themeTabLabel_color_normal
                        font: lightTab.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: themeSwitcher.currentIndex === 0 ? pal.HomePage_themeTabBg_color_active : "transparent"
                        radius: 5
                        anchors.fill: parent
                        anchors.margins: 3
                    }
                }

                TabButton {
                    id: darkTab
                    width: 60
                    text: "Dark"
                    font.pixelSize: 12
                    font.bold: themeSwitcher.currentIndex === 1
                    padding: 8

                    contentItem: Text {
                        text: darkTab.text
                        color: themeSwitcher.currentIndex === 1 ? pal.HomePage_themeTabLabel_color_active : pal.HomePage_themeTabLabel_color_normal
                        font: darkTab.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: themeSwitcher.currentIndex === 1 ? pal.HomePage_themeTabBg_color_active : "transparent"
                        radius: 5
                        anchors.fill: parent
                        anchors.margins: 3
                    }
                }

                onCurrentIndexChanged: {
                    if (currentIndex === 0) themeManager.setTheme("Light")
                    else themeManager.setTheme("Dark")
                }
            }
        }

        // 显示模式切换
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            Label {
                id: modeLabel
                text: "显示模式:"
                color: pal.HomePage_modeLabel_color
                font.pixelSize: 14
                Layout.alignment: Qt.AlignVCenter
            }

            RadioButton {
                id: showAllRadio
                text: "全部插件"
                checked: true
                onCheckedChanged: { if (checked) root.showAll = true; }

                contentItem: Text {
                    text: parent.text
                    color: pal.radioButton_textColor
                    font: parent.font
                    verticalAlignment: Text.AlignVCenter
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.indicator ? parent.indicator.right : parent.left
                    anchors.leftMargin: 8
                    anchors.right: parent.right
                }
            }

            RadioButton {
                id: categoryRadio
                text: "分类显示"
                onCheckedChanged: { if (checked) root.showAll = false; }

                contentItem: Text {
                    text: parent.text
                    color: pal.radioButton_textColor
                    font: parent.font
                    verticalAlignment: Text.AlignVCenter
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.indicator ? parent.indicator.right : parent.left
                    anchors.leftMargin: 8
                    anchors.right: parent.right
                }
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
                    id: categoryTabBarBg
                    color: pal.HomePage_categoryTabBarBg_color
                    radius: 8
                    border.color: pal.HomePage_categoryTabBarBorder_color
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
                            color: tabBar.currentIndex === index ? pal.HomePage_categoryTabLabel_color_active : pal.HomePage_categoryTabLabel_color_normal
                            font: tabBtn.font
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: tabBar.currentIndex === index ? pal.HomePage_categoryTabBg_color_active : tabBtn.hovered ? pal.HomePage_categoryTabBg_color_hover : "transparent"
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