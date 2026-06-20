import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Pane {
    id: root

    signal backRequested()

    property var controller: null
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
        color: pal.BatchRenamePage_rootBg_color
    }

    FolderDialog {
        id: folderDialog
        title: "选择源文件夹"
        onAccepted: {
            if (controller) {
                controller.rootPath = selectedFolder
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
                id: backConfirmMsg
                text: "当前有批量重命名任务正在处理中，返回首页将中断执行，是否继续？"
                color: pal.BatchRenamePage_backConfirmMsg_color
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
                    normalColor: pal.BatchRenamePage_cancelBtn_normalColor
                    hoverColor: pal.BatchRenamePage_cancelBtn_hoverColor
                    borderColor: pal.BatchRenamePage_cancelBtn_borderColor
                    textColor: pal.BatchRenamePage_cancelBtn_textColor
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: backConfirmDialog.close()
                }

                IconButton {
                    id: returnHomeBtn
                    text: "返回首页"
                    tooltip: "中断任务并返回首页"
                    normalColor: pal.BatchRenamePage_returnHomeBtn_normalColor
                    hoverColor: pal.BatchRenamePage_returnHomeBtn_hoverColor
                    borderColor: pal.BatchRenamePage_returnHomeBtn_borderColor
                    textColor: pal.BatchRenamePage_returnHomeBtn_textColor
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
                    text: "批量重命名"
                    color: pal.BatchRenamePage_titleLabel_color
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: subtitleLabel
                    text: "设置规则后直接执行，支持逐条还原"
                    color: pal.BatchRenamePage_subtitleLabel_color
                    font.pixelSize: 14
                }
            }
        }

        Rectangle {
            id: settingsPanel
            Layout.fillWidth: true
            Layout.fillHeight: false
            implicitHeight: settingsColumn.implicitHeight + 36
            radius: 10
            color: pal.BatchRenamePage_settingsPanel_color
            border.color: pal.BatchRenamePage_settingsPanel_borderColor
            border.width: 1

            ColumnLayout {
                id: settingsColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: rootFolderLabel
                        text: "源文件夹"
                        color: pal.BatchRenamePage_rootFolderLabel_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.rootPath : ""
                        readOnly: true
                        placeholderText: "点击选择文件夹"
                    }

                    CheckBoxEx {
                        id: recursiveCheck
                        implicitWidth: 110
                        text: "递归子文件夹"
                        textColor: pal.checkBox_textColor
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
                        id: fileTypeLabel
                        text: "文件类型"
                        color: pal.BatchRenamePage_fileTypeLabel_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        RadioButtonEx {
                            implicitWidth: 40
                            text: "所有"
                            textColor: pal.radioButton_textColor
                            checked: controller ? controller.fileType === 0 : true
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 0
                                }
                            }
                        }

                        RadioButtonEx {
                            implicitWidth: 40
                            text: "视频"
                            textColor: pal.radioButton_textColor
                            checked: controller ? controller.fileType === 1 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 1
                                }
                            }
                        }

                        RadioButtonEx {
                            implicitWidth: 40
                            text: "音频"
                            textColor: pal.radioButton_textColor
                            checked: controller ? controller.fileType === 2 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 2
                                }
                            }
                        }

                        RadioButtonEx {
                            implicitWidth: 40
                            text: "文本"
                            textColor: pal.radioButton_textColor
                            checked: controller ? controller.fileType === 3 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 3
                                }
                            }
                        }

                        RadioButtonEx {
                            implicitWidth: 40
                            text: "图片"
                            textColor: pal.radioButton_textColor
                            checked: controller ? controller.fileType === 4 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 4
                                }
                            }
                        }

                        RadioButtonEx {
                            id: customRadio
                            implicitWidth: 60
                            text: "自定义"
                            textColor: pal.radioButton_textColor
                            checked: controller ? controller.fileType === 5 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 5
                                }
                            }
                        }

                        TextFieldEx {
                            Layout.preferredWidth: 80
                            text: controller ? controller.customExtension : ""
                            placeholderText: ".txt"
                            enabled: controller ? controller.fileType === 5 : false
                            onTextChanged: {
                                if (controller) {
                                    controller.customExtension = text
                                }
                            }
                        }

                        Label {
                            id: fileTipsLabel
                            text: controller ? controller.fileTips : ""
                            color: pal.BatchRenamePage_fileTipsLabel_color
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
                        color: pal.BatchRenamePage_renameModeLabel_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RadioButtonEx {
                        id: specifyRadio
                        implicitWidth: 80
                        text: "指定名称"
                        textColor: pal.radioButton_textColor
                        checked: controller ? controller.renameMode === 0 : true
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.renameMode = 0
                            }
                        }
                    }

                    TextFieldEx {
                        Layout.preferredWidth: 150
                        text: controller ? controller.baseName : ""
                        placeholderText: "输入文件名"
                        enabled: specifyRadio.checked
                        onTextChanged: {
                            if (controller) {
                                controller.baseName = text
                            }
                        }
                    }

                    Rectangle {
                        id: modeSeparator
                        width: 1
                        height: 24
                        color: pal.BatchRenamePage_modeSeparator_color
                    }

                    RadioButtonEx {
                        id: replaceRadio
                        implicitWidth: 80
                        text: "替换文本"
                        textColor: pal.radioButton_textColor
                        checked: controller ? controller.renameMode === 1 : false
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.renameMode = 1
                            }
                        }
                    }

                    TextFieldEx {
                        Layout.preferredWidth: 120
                        text: controller ? controller.searchText : ""
                        placeholderText: "查找"
                        enabled: replaceRadio.checked
                        onTextChanged: {
                            if (controller) {
                                controller.searchText = text
                            }
                        }
                    }

                    Label {
                        id: arrowLabel
                        text: "→"
                        color: pal.BatchRenamePage_arrowLabel_color
                    }

                    TextFieldEx {
                        Layout.preferredWidth: 120
                        text: controller ? controller.replaceText : ""
                        placeholderText: "替换"
                        enabled: replaceRadio.checked
                        onTextChanged: {
                            if (controller) {
                                controller.replaceText = text
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
                        tooltip: "开始批量重命名"
                        normalColor: pal.BatchRenamePage_executeBtn_normalColor
                        hoverColor: pal.BatchRenamePage_executeBtn_hoverColor
                        borderColor: pal.BatchRenamePage_executeBtn_borderColor
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
            spacing: 12
            visible: (controller && controller.statusMessage.length > 0) || (controller && controller.hasRecords)

            Rectangle {
                id: statusPanel
                Layout.fillWidth: true
                height: statusText.implicitHeight + 12
                radius: 6
                color: controller && controller.statusMessage ? pal.BatchRenamePage_statusPanel_bg_active : "transparent"
                visible: controller ? controller.statusMessage.length > 0 : false

                Label {
                    id: statusText
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    text: controller ? controller.statusMessage : ""
                    color: pal.BatchRenamePage_statusText_color
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
            }

            IconButton {
                iconSource: "qrc:/icons/undo.svg"
                tooltip: "批量还原"
                visible: controller ? controller.hasRecords : false
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
            color: pal.BatchRenamePage_tablePanel_color
            border.color: pal.BatchRenamePage_tablePanel_borderColor
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
                    color: pal.BatchRenamePage_headerRow_color
                    radius: 0

                    // 底部分隔线
                    Rectangle {
                        id: headerSeparator
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: pal.BatchRenamePage_headerSeparator_color
                    }

                    Label {
                        id: typeHeader
                        x: root.typeColumnX
                        width: root.typeColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "类型"
                        color: pal.BatchRenamePage_typeHeader_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: originalHeader
                        x: root.originalColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "原名称"
                        color: pal.BatchRenamePage_originalHeader_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: newHeader
                        x: root.newColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "新名称"
                        color: pal.BatchRenamePage_newHeader_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: statusHeader
                        x: root.statusColumnX
                        width: root.statusColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "状态"
                        color: pal.BatchRenamePage_statusHeader_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: actionHeader
                        x: root.actionColumnX
                        width: root.actionColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "操作"
                        color: pal.BatchRenamePage_actionHeader_color
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
                        color: rowMouseArea.containsMouse ? pal.BatchRenamePage_rowDelegate_bg_hover :
                               index % 2 === 0 ? pal.BatchRenamePage_rowDelegate_bg_even : pal.BatchRenamePage_rowDelegate_bg_odd

                        // 底部分隔线
                        Rectangle {
                            id: rowSeparator
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: pal.BatchRenamePage_rowSeparator_color
                        }

                        // 类型标签（彩色标签）
                        Rectangle {
                            x: root.typeColumnX
                            width: root.typeColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: modelData.fileType && modelData.fileType.startsWith("视频") ? "#f3e8ff" :
                                   modelData.fileType && modelData.fileType.startsWith("图片") ? "#d1fae5" :
                                   modelData.fileType && modelData.fileType.startsWith("音频") ? "#fef3c7" :
                                   modelData.fileType && modelData.fileType.startsWith("文本") ? "#dbeafe" : "#f1f5f9"

                            Label {
                                anchors.centerIn: parent
                                text: modelData.fileType
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.fileType && modelData.fileType.startsWith("视频") ? "#7c3aed" :
                                       modelData.fileType && modelData.fileType.startsWith("图片") ? "#0f766e" :
                                       modelData.fileType && modelData.fileType.startsWith("音频") ? "#b45309" :
                                       modelData.fileType && modelData.fileType.startsWith("文本") ? "#2563eb" : "#64748b"
                            }
                        }

                        Label {
                            id: originalNameLabel
                            x: root.originalColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: modelData.originalName
                            color: pal.BatchRenamePage_originalNameLabel_color
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        Label {
                            id: newNameLabel
                            x: root.newColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: modelData.newName
                            color: pal.BatchRenamePage_newNameLabel_color
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
                            color: modelData.success ? pal.BatchRenamePage_statusBadge_bg_active : pal.BatchRenamePage_statusBadge_bg_normal

                            Label {
                                id: statusBadgeLabel
                                anchors.centerIn: parent
                                text: modelData.status
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.success ? pal.BatchRenamePage_statusBadgeLabel_color_active : pal.BatchRenamePage_statusBadgeLabel_color_normal
                            }
                        }

                        Label {
                            id: pathLabel
                            x: root.originalColumnX
                            y: 40
                            width: root.statusColumnX - root.originalColumnX - root.columnGap
                            text: (modelData.success ? modelData.newPath : modelData.originalPath)
                            color: pal.BatchRenamePage_pathLabel_color
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }

                        IconButton {
                            x: root.actionColumnX
                            width: root.actionColumnWidth
                            anchors.verticalCenter: parent.verticalCenter
                            iconSource: "qrc:/icons/undo.svg"
                            tooltip: modelData.success ? "还原" : "失败项无法还原"
                            enabled: modelData.success
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
                            color: pal.BatchRenamePage_emptyTitleLabel_color
                            font.pixelSize: 15
                        }
                        Label {
                            id: emptySubtitleLabel
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "设置规则后点击执行按钮开始"
                            color: pal.BatchRenamePage_emptySubtitleLabel_color
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}