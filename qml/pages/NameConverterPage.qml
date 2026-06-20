import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Pane {
    id: root
    property var pal: themeManager.palette

    signal backRequested()

    property var controller: null

    readonly property int tableLeftPadding: 18
    readonly property int tableRightPadding: 20
    readonly property int typeColumnWidth: 74
    readonly property int statusColumnWidth: 118
    readonly property int actionColumnWidth: 44
    readonly property int columnGap: 12
    readonly property int textColumnWidth: Math.max(160, (previewList.width
        - tableLeftPadding
        - tableRightPadding
        - typeColumnWidth
        - statusColumnWidth
        - actionColumnWidth
        - columnGap * 4) / 2)
    readonly property int typeColumnX: tableLeftPadding
    readonly property int currentNameColumnX: typeColumnX + typeColumnWidth + columnGap
    readonly property int newNameColumnX: currentNameColumnX + textColumnWidth + columnGap
    readonly property int statusColumnX: newNameColumnX + textColumnWidth + columnGap
    readonly property int actionColumnX: statusColumnX + statusColumnWidth + columnGap

    padding: 0
    background: Rectangle {
        id: rootBg
        color: pal.NameConverter_rootBg_color
    }

    FolderDialog {
        id: folderDialog
        title: "选择源文件夹"
        onAccepted: {
            if (controller) {
                controller.rootPath = selectedFolder
                controller.clearRecords()
            }
        }
    }

    // ── 任务执行中返回确认对话框 ─────────────────────────────────────
    Dialog {
        id: backConfirmDialog
        title: "确认返回"
        modal: true
        anchors.centerIn: parent
        width: 400
        standardButtons: Dialog.NoButton
        closePolicy: Dialog.CloseOnEscape

        contentItem: ColumnLayout {
            spacing: 8
            Layout.margins: 4

            Label {
                id: confirmMsgLabel
                text: "当前有繁转简任务正在处理中，返回首页将中断执行，是否继续？"
                color: pal.NameConverter_confirmMsgLabel_color
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.bottomMargin: 8
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Item { Layout.fillWidth: true }

                IconButton {
                    id: cancelBtn
                    text: "取消"
                    tooltip: "不返回，继续当前任务"
                    normalColor: pal.NameConverter_cancelBtn_normalColor
                    hoverColor: pal.NameConverter_cancelBtn_hoverColor
                    borderColor: pal.NameConverter_cancelBtn_borderColor
                    textColor: pal.NameConverter_cancelBtn_textColor
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: backConfirmDialog.close()
                }

                IconButton {
                    id: confirmBackBtn
                    text: "返回首页"
                    tooltip: "中断任务并返回首页"
                    normalColor: pal.NameConverter_confirmBackBtn_normalColor
                    hoverColor: pal.NameConverter_confirmBackBtn_hoverColor
                    borderColor: pal.NameConverter_confirmBackBtn_borderColor
                    textColor: pal.NameConverter_confirmBackBtn_textColor
                    implicitWidth: 120
                    implicitHeight: 38
                    onClicked: {
                        if (controller) { controller.cancel(); }
                        backConfirmDialog.close();
                        root.backRequested();
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            IconButton {
                iconSource: "qrc:/icons/arrow-left.svg"
                implicitHeight: 38
                tooltip: "返回"
                onClicked: {
                    if (controller && controller.isProcessing) {
                        backConfirmDialog.open();
                    } else {
                        root.backRequested();
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    id: titleLabel
                    text: "文件名繁转简"
                    color: pal.NameConverter_titleLabel_color
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: descLabel
                    text: "选择源文件夹后直接执行转换，完成后可按记录逐条还原"
                    color: pal.NameConverter_descLabel_color
                    font.pixelSize: 14
                }
            }
        }

        Rectangle {
            id: sourcePanel
            Layout.fillWidth: true
            Layout.fillHeight: false
            implicitHeight: settingsColumn.implicitHeight + 36
            radius: 10
            color: pal.NameConverter_sourcePanel_color
            border.color: pal.NameConverter_sourcePanel_borderColor
            border.width: 1

            // 面板阴影
            Rectangle {
                id: sourceShadow
                anchors.fill: parent
                anchors.margins: -2
                radius: 12
                color: pal.NameConverter_sourceShadow_color
                opacity: 0.04
                z: -1
            }

            ColumnLayout {
                id: settingsColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: sourceFolderLabel
                        text: "源文件夹"
                        color: pal.NameConverter_sourceFolderLabel_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.rootPath : ""
                        readOnly: true
                        placeholderText: "尚未选择"
                    }

                    CheckBox {
                        id: recursiveCheck
                        Layout.preferredWidth: 110
                        text: "递归子文件夹"
                        checked: controller ? controller.recursive : false
                        onCheckedChanged: {
                            if (controller) {
                                controller.recursive = checked
                            }
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择源文件夹"
                        onClicked: folderDialog.open()
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: processTypeLabel
                        Layout.preferredWidth: 80
                        text: "处理类型"
                        color: pal.NameConverter_processTypeLabel_color
                        font.pixelSize: 13
                        font.bold: true
                    }

                    ComboBoxEx {
                        id: processTypeCombo
                        Layout.preferredWidth: 140
                        model: ["仅文件", "仅文件夹", "文件和文件夹"]
                        currentIndex: controller ? controller.targetType : 2
                        onActivated: {
                            if (controller) {
                                controller.targetType = currentIndex
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    IconButton {
                        iconSource: "qrc:/icons/trash.svg"
                        tooltip: "清空记录"
                        visible: controller ? controller.hasRecords : false
                        onClicked: {
                            if (controller) {
                                controller.clearRecords()
                            }
                        }
                    }

                    IconButton {
                        id: executeBtn
                        implicitWidth: 150
                        text: "开始处理"
                        iconSource: "qrc:/icons/play.svg"
                        tooltip: "开始文本繁简转换"
                        normalColor: pal.NameConverter_executeBtn_normalColor
                        hoverColor: pal.NameConverter_executeBtn_hoverColor
                        borderColor: pal.NameConverter_executeBtn_borderColor
                        onClicked: {
                            if (controller) {
                                controller.executeRename()
                            }
                        }
                    }
                }
            }
        }

        // 状态栏
        Rectangle {
            id: statusBar
            Layout.fillWidth: true
            height: statusText.implicitHeight + 12
            radius: 6
            color: pal.NameConverter_statusBar_color
            visible: controller ? controller.statusMessage.length > 0 : false

            Label {
                id: statusText
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 12
                text: controller ? controller.statusMessage : ""
                color: pal.NameConverter_statusText_color
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }

        Rectangle {
            id: previewPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: pal.NameConverter_previewPanel_color
            border.color: pal.NameConverter_previewPanel_borderColor
            border.width: 1
            clip: true

            // 面板阴影
            Rectangle {
                id: previewShadow
                anchors.fill: parent
                anchors.margins: -2
                radius: 12
                color: pal.NameConverter_previewShadow_color
                opacity: 0.04
                z: -1
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 2
                spacing: 0

                Rectangle {
                    id: headerRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: pal.NameConverter_headerRow_color

                    // 底部分隔线
                    Rectangle {
                        id: headerSeparator
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: pal.NameConverter_headerSeparator_color
                    }

                    Label {
                        id: typeHeader
                        x: root.typeColumnX
                        width: root.typeColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "类型"
                        color: pal.NameConverter_typeHeader_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: currentNameHeader
                        x: root.currentNameColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "原名称"
                        color: pal.NameConverter_currentNameHeader_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: newNameHeader
                        x: root.newNameColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "新名称"
                        color: pal.NameConverter_newNameHeader_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: statusHeader
                        x: root.statusColumnX
                        width: root.statusColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "状态"
                        color: pal.NameConverter_statusHeader_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: actionHeader
                        x: root.actionColumnX
                        width: root.actionColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "操作"
                        color: pal.NameConverter_actionHeader_color
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                ListView {
                    id: previewList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: controller ? controller.previewModel : null
                    clip: true
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AlwaysOn
                    }

                    delegate: Rectangle {
                        id: rowDelegate
                        width: previewList.width
                        height: 74
                        color: rowMouseArea.containsMouse ? pal.NameConverter_rowDelegate_bg_hover :
                               index % 2 === 0 ? pal.NameConverter_rowDelegate_bg_even : pal.NameConverter_rowDelegate_bg_odd

                        // 底部分隔线
                        Rectangle {
                            id: rowSeparator
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: pal.NameConverter_rowSeparator_color
                        }

                        // 类型标签（彩色标签）
                        Rectangle {
                            x: root.typeColumnX
                            width: root.typeColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: directory ? "#f3e8ff" : "#d1fae5"

                            Label {
                                anchors.centerIn: parent
                                text: directory ? "文件夹" : "文件"
                                font.pixelSize: 11
                                font.bold: true
                                color: directory ? "#7c3aed" : "#0f766e"
                            }
                        }

                        Label {
                            id: currentNameLabel
                            x: root.currentNameColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: currentName
                            color: pal.NameConverter_currentNameLabel_color
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        Label {
                            id: newNameLabel
                            x: root.newNameColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: newName
                            color: pal.NameConverter_newNameLabel_color
                            font.bold: true
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        // 状态标签（彩色标签）
                        Rectangle {
                            id: statusRect
                            x: root.statusColumnX
                            width: root.statusColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: status.indexOf("失败") === 0 ? pal.NameConverter_statusRect_bg_fail :
                                   status === "已还原" ? pal.NameConverter_statusRect_bg_restored : pal.NameConverter_statusRect_bg_done

                            Label {
                                id: statusLabel
                                anchors.centerIn: parent
                                text: status
                                font.pixelSize: 11
                                font.bold: true
                                color: status.indexOf("失败") === 0 ? pal.NameConverter_statusLabel_color_fail :
                                       status === "已还原" ? pal.NameConverter_statusLabel_color_restored : pal.NameConverter_statusLabel_color_done
                                elide: Text.ElideRight
                            }
                        }

                        Label {
                            id: pathLabel
                            x: root.currentNameColumnX
                            y: 40
                            width: root.statusColumnX - root.currentNameColumnX - root.columnGap
                            text: actualPath
                            color: pal.NameConverter_pathLabel_color
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }

                        IconButton {
                            x: root.actionColumnX
                            width: root.actionColumnWidth
                            anchors.verticalCenter: parent.verticalCenter
                            iconSource: "qrc:/icons/undo.svg"
                            tooltip: status.indexOf("失败") === 0 ? "失败项无法还原" : "还原"
                            enabled: status === "已转换"
                            onClicked: {
                                if (controller) {
                                    controller.restoreRecord(index)
                                }
                            }
                        }

                        MouseArea {
                            id: rowMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    // 空状态
                    Column {
                        anchors.centerIn: parent
                        spacing: 8
                        visible: previewList.count === 0

                        Label {
                            id: emptyTitleLabel
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "暂无转换记录"
                            color: pal.NameConverter_emptyTitleLabel_color
                            font.pixelSize: 15
                        }
                        Label {
                            id: emptyDescLabel
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "选择文件夹后点击执行按钮开始"
                            color: pal.NameConverter_emptyDescLabel_color
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}