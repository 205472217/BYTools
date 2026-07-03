import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Pane {
    id: root

    signal backRequested()

    property var controller: null
    property var stackView: null
    property string pluginId: ""
    property var pal: themeManager.palette

    readonly property int tableLeftPadding: 18
    readonly property int tableRightPadding: 20
    readonly property int typeColumnWidth: 74
    readonly property int statusColumnWidth: 118
    readonly property int actionColumnWidth: 44
    readonly property int columnGap: 12
    readonly property int textColumnWidth: Math.max(160, (recordsListView.width
        - tableLeftPadding
        - tableRightPadding
        - typeColumnWidth
        - statusColumnWidth
        - actionColumnWidth
        - columnGap * 4) / 2)
    readonly property int typeColumnX: tableLeftPadding
    readonly property int originalColumnX: typeColumnX + typeColumnWidth + columnGap
    readonly property int newColumnX: originalColumnX + textColumnWidth + columnGap
    readonly property int statusColumnX: newColumnX + textColumnWidth + columnGap
    readonly property int actionColumnX: statusColumnX + statusColumnWidth + columnGap

    padding: 0
    background: Rectangle {
        id: rootBg
        color: pal.SurfaceEx_pageBg
    }

    FolderDialog {
        id: folderDialog
        title: "选择源文件夹"
        onAccepted: {
            if (controller) {
                controller.rootPath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""))
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
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: Label {
            text: "当前有批量重命名任务正在处理中，返回首页将中断执行，是否继续？"
            color: pal.LabelEx_statusText
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            focus: true
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    backConfirmDialog.accept()
                    event.accepted = true
                }
            }
        }

        onAccepted: {
            if (controller) { controller.reset(); }
            root.backRequested();
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
                        id: backBtn
                        iconSource: "qrc:/icons/global_back.svg"
                        implicitHeight: 38
                        tooltip: "返回"
                        paletteGroup: "IconBtnEx"
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
                    text: "批量重命名"
                    color: pal.LabelEx_titleText
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: subtitleLabel
                    text: "设置规则后直接执行，支持逐条还原"
                    color: pal.LabelEx_subtitleText
                    font.pixelSize: 14
                }
            }
        }

        Rectangle {
            id: controllerPanel
            Layout.fillWidth: true
            Layout.fillHeight: false
            implicitHeight: controllerColumn.implicitHeight + 36
            radius: 10
            color: pal.SurfaceEx_cardBg
            border.color: pal.SurfaceEx_cardBorder
            border.width: 1

            ColumnLayout {
                id: controllerColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: rootFolderLabel
                        text: "源文件夹"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        id: rootPathField
                        Layout.fillWidth: true
                        text: controller ? controller.rootPath : ""
                        readOnly: true
                        placeholderText: "点击选择文件夹"
                        paletteGroup: "TextFieldEx"
                    }

                    CheckBoxEx {
                        id: recursiveCheck
                        implicitWidth: 110
                        text: "递归子文件夹"
                        paletteGroup: "CheckBoxEx"
                        checked: controller ? controller.recursive : false
                        onCheckedChanged: {
                            if (controller && controller.recursive !== checked)
                                controller.recursive = checked
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/global_folder.svg"
                        tooltip: "选择源文件夹"
                        paletteGroup: "IconBtnEx"
                        onClicked: folderDialog.open()
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: fileTypeLabel
                        text: "文件类型"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        RadioButtonEx {
                            id: allTypeRadio
                            implicitWidth: 40
                            text: "所有"
                            paletteGroup: "RadioButtonEx"
                            checked: controller ? controller.fileType === 0 : true
                            onCheckedChanged: {
                                if (checked && controller && controller.fileType !== 0)
                                    controller.fileType = 0
                            }
                        }

                        RadioButtonEx {
                            id: videoTypeRadio
                            implicitWidth: 40
                            text: "视频"
                            paletteGroup: "RadioButtonEx"
                            checked: controller ? controller.fileType === 1 : false
                            onCheckedChanged: {
                                if (checked && controller && controller.fileType !== 1)
                                    controller.fileType = 1
                            }
                        }

                        RadioButtonEx {
                            id: audioTypeRadio
                            implicitWidth: 40
                            text: "音频"
                            paletteGroup: "RadioButtonEx"
                            checked: controller ? controller.fileType === 2 : false
                            onCheckedChanged: {
                                if (checked && controller && controller.fileType !== 2)
                                    controller.fileType = 2
                            }
                        }

                        RadioButtonEx {
                            id: textTypeRadio
                            implicitWidth: 40
                            text: "文本"
                            paletteGroup: "RadioButtonEx"
                            checked: controller ? controller.fileType === 3 : false
                            onCheckedChanged: {
                                if (checked && controller && controller.fileType !== 3)
                                    controller.fileType = 3
                            }
                        }

                        RadioButtonEx {
                            id: imageTypeRadio
                            implicitWidth: 40
                            text: "图片"
                            paletteGroup: "RadioButtonEx"
                            checked: controller ? controller.fileType === 4 : false
                            onCheckedChanged: {
                                if (checked && controller && controller.fileType !== 4)
                                    controller.fileType = 4
                            }
                        }

                        RadioButtonEx {
                            id: customRadio
                            implicitWidth: 60
                            text: "自定义"
                            paletteGroup: "RadioButtonEx"
                            checked: controller ? controller.fileType === 5 : false
                            onCheckedChanged: {
                                if (checked && controller && controller.fileType !== 5)
                                    controller.fileType = 5
                            }
                        }

                        TextFieldEx {
                            id: customExtField
                            Layout.preferredWidth: 80
                            text: controller ? controller.customExtension : ""
                            placeholderText: ".txt"
                            enabled: controller ? controller.fileType === 5 : false
                            paletteGroup: "TextFieldEx"
                            onTextChanged: {
                                if (controller) {
                                    controller.customExtension = text
                                }
                            }
                        }

                        Label {
                            id: fileTipsLabel
                            text: ""
                            color: pal.LabelEx_infoText
                            font.pixelSize: 12
                            visible: controller ? controller.fileType !== 5 : false
                        }
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: renameModeLabel
                        text: "重命名方式"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RadioButtonEx {
                        id: replaceRadio
                        implicitWidth: 80
                        text: "替换文本"
                        paletteGroup: "RadioButtonEx"
                        checked: controller ? controller.renameMode === 1 : false
                        onCheckedChanged: {
                            if (checked && controller && controller.renameMode !== 1)
                                controller.renameMode = 1
                        }
                    }

                    TextFieldEx {
                        id: searchTextField
                        Layout.fillWidth: true
                        text: controller ? controller.searchText : ""
                        placeholderText: "查找"
                        enabled: replaceRadio.checked
                        paletteGroup: "TextFieldEx"
                        onTextChanged: {
                            if (controller) {
                                controller.searchText = text
                            }
                        }
                    }

                    Label {
                        id: arrowLabel
                        text: "→"
                        color: pal.LabelEx_infoText
                    }

                    TextFieldEx {
                        id: replaceTextField
                        Layout.fillWidth: true
                        text: controller ? controller.replaceText : ""
                        placeholderText: "替换"
                        enabled: replaceRadio.checked
                        paletteGroup: "TextFieldEx"
                        onTextChanged: {
                            if (controller) {
                                controller.replaceText = text
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: renameModeKeep
                        text: ""
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RadioButtonEx {
                        id: specifyRadio
                        implicitWidth: 80
                        text: "指定名称"
                        paletteGroup: "RadioButtonEx"
                        checked: controller ? controller.renameMode === 0 : true
                        onCheckedChanged: {
                            if (checked && controller && controller.renameMode !== 0)
                                controller.renameMode = 0
                        }
                    }

                    TextFieldEx {
                        id: baseNameField
                        Layout.fillWidth: true
                        text: controller ? controller.baseName : ""
                        placeholderText: "输入文件名"
                        enabled: specifyRadio.checked
                        paletteGroup: "TextFieldEx"
                        onTextChanged: {
                            if (controller) {
                                controller.baseName = text
                            }
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/global_trash.svg"
                        tooltip: "清空记录"
                        visible: controller ? controller.hasRecords : false
                        paletteGroup: "IconBtnEx"
                        onClicked: {
                            if (controller) {
                                controller.clearRecords()
                            }
                        }
                    }

                    IconButton {
                        id: executeBtn
                        implicitWidth: 110
                        text: "开始处理"
                        iconSource: "qrc:/icons/global_start.svg"
                        tooltip: "开始批量重命名"
                        paletteGroup: "BatchRenamePage_executeBtn"
                        onClicked: {
                            if (controller) {
                                controller.executeRename()
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            spacing: 12
            visible: true

            Rectangle {
                id: statusPanel
                Layout.fillWidth: true
                height: statusText.implicitHeight + 12
                radius: 6
                color: controller && controller.statusMessage ? pal.SurfaceEx_statusBar : "transparent"
                visible: true

                Label {
                    id: statusText
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    text: controller && controller.statusMessage.length > 0
                        ? controller.statusMessage
                        : "就绪"
                    color: pal.LabelEx_statusText
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
            }

            IconButton {
                iconSource: "qrc:/icons/global_undo.svg"
                tooltip: "批量还原"
                visible: controller ? controller.hasRecords : false
                paletteGroup: "IconBtnEx"
                onClicked: {
                    if (controller) {
                        controller.restoreAllRecords()
                    }
                }
            }
        }

        Rectangle {
            id: tablePanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: pal.SurfaceEx_cardBg
            border.color: pal.SurfaceEx_cardBorder
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 2
                spacing: 0

                Rectangle {
                    id: headerRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: pal.SurfaceEx_headerRowBg
                    radius: 0

                    // 底部分隔线
                    Rectangle {
                        id: headerSeparator
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: pal.SurfaceEx_headerDivider
                    }

                    Label {
                        id: typeHeader
                        x: root.typeColumnX
                        width: root.typeColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "类型"
                        color: pal.LabelEx_headerText
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: originalHeader
                        x: root.originalColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "原名称"
                        color: pal.LabelEx_headerText
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: newHeader
                        x: root.newColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "新名称"
                        color: pal.LabelEx_headerText
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: statusHeader
                        x: root.statusColumnX
                        width: root.statusColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "状态"
                        color: pal.LabelEx_headerText
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: actionHeader
                        x: root.actionColumnX
                        width: root.actionColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "操作"
                        color: pal.LabelEx_headerText
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                ListView {
                    id: recordsListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 200
                    model: controller ? controller.records : null
                    clip: true
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AlwaysOn
                    }

                    delegate: Rectangle {
                        id: rowDelegate
                        width: recordsListView.width
                        height: 74
                        color: rowMouseArea.containsMouse ? pal.SurfaceEx_rowHoverBg :
                               index % 2 === 0 ? pal.SurfaceEx_rowEvenBg : pal.SurfaceEx_rowOddBg

                        // 底部分隔线
                        Rectangle {
                            id: rowSeparator
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: pal.SurfaceEx_rowDivider
                        }

                        // 类型标签（彩色标签）
                        Rectangle {
                            x: root.typeColumnX
                            width: root.typeColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: modelData.fileType && modelData.fileType.startsWith("视频") ? pal.LabelEx_Video_BgRect :
                                   modelData.fileType && modelData.fileType.startsWith("图片") ? pal.LabelEx_Image_BgRect :
                                   modelData.fileType && modelData.fileType.startsWith("音频") ? pal.LabelEx_Audio_BgRect :
                                   modelData.fileType && modelData.fileType.startsWith("文本") ? pal.LabelEx_Doc_BgRect : pal.LabelEx_Other_BgRect

                            Label {
                                anchors.centerIn: parent
                                text: modelData.fileType
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.fileType && modelData.fileType.startsWith("视频") ? pal.LabelEx_Video_Text :
                                       modelData.fileType && modelData.fileType.startsWith("图片") ? pal.LabelEx_Image_Text :
                                       modelData.fileType && modelData.fileType.startsWith("音频") ? pal.LabelEx_Audio_Text :
                                       modelData.fileType && modelData.fileType.startsWith("文本") ? pal.LabelEx_Doc_Text : pal.LabelEx_Other_Text
                            }
                        }

                        Label {
                            id: originalNameLabel
                            x: root.originalColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: modelData.originalName
                            color: pal.LabelEx_valueText
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        Label {
                            id: newNameLabel
                            x: root.newColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: modelData.newName
                            color: pal.LabelEx_successText
                            font.bold: true
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        // 状态标签（彩色标签）
                        Rectangle {
                            id: statusBadge
                            x: root.statusColumnX
                            width: root.statusColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: modelData.success ? pal.StatusBadgeEx_bg_success : pal.StatusBadgeEx_bg_error

                            Label {
                                id: statusBadgeLabel
                                anchors.centerIn: parent
                                text: modelData.status
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.success ? pal.LabelEx_statusText : pal.LabelEx_errorText
                            }
                        }

                        Label {
                            id: pathLabel
                            x: root.originalColumnX
                            y: 40
                            width: root.statusColumnX - root.originalColumnX - root.columnGap
                            text: (modelData.success ? modelData.newPath : modelData.originalPath)
                            color: pal.LabelEx_pathText
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }

                        IconButton {
                            x: root.actionColumnX
                            width: root.actionColumnWidth
                            anchors.verticalCenter: parent.verticalCenter
                            iconSource: "qrc:/icons/global_undo.svg"
                            tooltip: modelData.success ? "还原" : "失败项无法还原"
                            enabled: modelData.success
                            paletteGroup: "IconBtnEx"
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
                        visible: controller ? recordsListView.count === 0 : true

                        Label {
                            id: emptyTitleLabel
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "暂无重命名记录"
                            color: pal.LabelEx_infoText
                            font.pixelSize: 15
                        }
                        Label {
                            id: emptySubtitleLabel
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "设置规则后点击执行按钮开始"
                            color: pal.LabelEx_infoText
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }

}

