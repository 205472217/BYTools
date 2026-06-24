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
    property QtObject settings: pluginManager.getPluginSettings("batch-rename")

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
            if (settings) {
                settings.rootPath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""))
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
                color: pal.LabelEx_statusText
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
                        id: backBtn
                        iconSource: "qrc:/icons/arrow-left.svg"
                        implicitHeight: 38
                        tooltip: "返回"
                        normalColor: pal.IconBtnEx_normalColor
                        hoverColor: pal.IconBtnEx_hoverColor
                        pressColor: pal.IconBtnEx_pressColor
                        borderColor: pal.IconBtnEx_borderColor
                        defaultBorderColor: pal.IconBtnEx_defaultBorderColor
                        disabledColor: pal.IconBtnEx_disabledColor
                        disabledBorderColor: pal.IconBtnEx_disabledBorderColor
                        textColor: pal.IconBtnEx_textColor
                        disabledTextColor: pal.IconBtnEx_disabledTextColor
                        shadowColor: pal.IconBtnEx_shadowColor
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
            id: settingsPanel
            Layout.fillWidth: true
            Layout.fillHeight: false
            implicitHeight: settingsColumn.implicitHeight + 36
            radius: 10
            color: pal.SurfaceEx_cardBg
            border.color: pal.SurfaceEx_cardBorder
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
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        id: rootPathField
                        Layout.fillWidth: true
                        text: settings ? settings.rootPath : ""
                        readOnly: true
                        placeholderText: "点击选择文件夹"
                        bgColor: pal.TextFieldEx_bgColor
                        textColor: pal.TextFieldEx_textColor
                        phColor: pal.TextFieldEx_phColor
                        selColor: pal.TextFieldEx_selColor
                        selTextColor: pal.TextFieldEx_selTextColor
                        borderColor: pal.TextFieldEx_borderColor
                        focusBorderColor: pal.TextFieldEx_focusBorderColor
                        focusRingColor: pal.TextFieldEx_focusRingColor
                        cursorColor: pal.TextFieldEx_cursorColor
                    }

                    CheckBoxEx {
                        id: recursiveCheck
                        implicitWidth: 110
                        text: "递归子文件夹"
                        textColor: pal.CheckBoxEx_textColor
                        checked: settings ? settings.recursive : false
                        onCheckedChanged: {
                            if (settings) {
                                settings.recursive = checked
                            }
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择源文件夹"
                        normalColor: pal.IconBtnEx_normalColor
                        hoverColor: pal.IconBtnEx_hoverColor
                        pressColor: pal.IconBtnEx_pressColor
                        borderColor: pal.IconBtnEx_borderColor
                        defaultBorderColor: pal.IconBtnEx_defaultBorderColor
                        disabledColor: pal.IconBtnEx_disabledColor
                        disabledBorderColor: pal.IconBtnEx_disabledBorderColor
                        textColor: pal.IconBtnEx_textColor
                        disabledTextColor: pal.IconBtnEx_disabledTextColor
                        shadowColor: pal.IconBtnEx_shadowColor
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
                            textColor: pal.RadioButtonEx_textColor
                            checked: settings ? settings.fileType === 0 : true
                            onCheckedChanged: {
                                if (checked && settings) {
                                    settings.fileType = 0
                                }
                            }
                        }

                        RadioButtonEx {
                            id: videoTypeRadio
                            implicitWidth: 40
                            text: "视频"
                            textColor: pal.RadioButtonEx_textColor
                            checked: settings ? settings.fileType === 1 : false
                            onCheckedChanged: {
                                if (checked && settings) {
                                    settings.fileType = 1
                                }
                            }
                        }

                        RadioButtonEx {
                            id: audioTypeRadio
                            implicitWidth: 40
                            text: "音频"
                            textColor: pal.RadioButtonEx_textColor
                            checked: settings ? settings.fileType === 2 : false
                            onCheckedChanged: {
                                if (checked && settings) {
                                    settings.fileType = 2
                                }
                            }
                        }

                        RadioButtonEx {
                            id: textTypeRadio
                            implicitWidth: 40
                            text: "文本"
                            textColor: pal.RadioButtonEx_textColor
                            checked: settings ? settings.fileType === 3 : false
                            onCheckedChanged: {
                                if (checked && settings) {
                                    settings.fileType = 3
                                }
                            }
                        }

                        RadioButtonEx {
                            id: imageTypeRadio
                            implicitWidth: 40
                            text: "图片"
                            textColor: pal.RadioButtonEx_textColor
                            checked: settings ? settings.fileType === 4 : false
                            onCheckedChanged: {
                                if (checked && settings) {
                                    settings.fileType = 4
                                }
                            }
                        }

                        RadioButtonEx {
                            id: customRadio
                            implicitWidth: 60
                            text: "自定义"
                            textColor: pal.RadioButtonEx_textColor
                            checked: settings ? settings.fileType === 5 : false
                            onCheckedChanged: {
                                if (checked && settings) {
                                    settings.fileType = 5
                                }
                            }
                        }

                        TextFieldEx {
                            id: customExtField
                            Layout.preferredWidth: 80
                            text: settings ? settings.customExtension : ""
                            placeholderText: ".txt"
                            enabled: settings ? settings.fileType === 5 : false
                            onTextChanged: {
                                if (settings) {
                                    settings.customExtension = text
                                }
                            }
                            bgColor: pal.TextFieldEx_bgColor
                            disabledBgColor: pal.TextFieldEx_disabledBgColor
                            textColor: pal.TextFieldEx_textColor
                            disabledTextColor: pal.TextFieldEx_disabledTextColor
                            phColor: pal.TextFieldEx_phColor
                            selColor: pal.TextFieldEx_selColor
                            selTextColor: pal.TextFieldEx_selTextColor
                            borderColor: pal.TextFieldEx_borderColor
                            disabledBorderColor: pal.TextFieldEx_disabledBorderColor
                            focusBorderColor: pal.TextFieldEx_focusBorderColor
                            focusRingColor: pal.TextFieldEx_focusRingColor
                            cursorColor: pal.TextFieldEx_cursorColor
                        }

                        Label {
                            id: fileTipsLabel
                            text: ""
                            color: pal.LabelEx_infoText
                            font.pixelSize: 12
                            visible: settings ? settings.fileType !== 5 : false
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
                        id: specifyRadio
                        implicitWidth: 80
                        text: "指定名称"
                        textColor: pal.RadioButtonEx_textColor
                        checked: settings ? settings.renameMode === 0 : true
                        onCheckedChanged: {
                            if (checked && settings) {
                                settings.renameMode = 0
                            }
                        }
                    }

                    TextFieldEx {
                        id: baseNameField
                        Layout.preferredWidth: 150
                        text: settings ? settings.baseName : ""
                        placeholderText: "输入文件名"
                        enabled: specifyRadio.checked
                        onTextChanged: {
                            if (settings) {
                                settings.baseName = text
                            }
                        }
                        bgColor: pal.TextFieldEx_bgColor
                        disabledBgColor: pal.TextFieldEx_disabledBgColor
                        textColor: pal.TextFieldEx_textColor
                        disabledTextColor: pal.TextFieldEx_disabledTextColor
                        phColor: pal.TextFieldEx_phColor
                        selColor: pal.TextFieldEx_selColor
                        selTextColor: pal.TextFieldEx_selTextColor
                        borderColor: pal.TextFieldEx_borderColor
                        disabledBorderColor: pal.TextFieldEx_disabledBorderColor
                        focusBorderColor: pal.TextFieldEx_focusBorderColor
                        focusRingColor: pal.TextFieldEx_focusRingColor
                        cursorColor: pal.TextFieldEx_cursorColor
                    }

                    Rectangle {
                        id: modeSeparator
                        width: 1
                        height: 24
                        color: pal.SurfaceEx_divider
                    }

                    RadioButtonEx {
                        id: replaceRadio
                        implicitWidth: 80
                        text: "替换文本"
                        textColor: pal.RadioButtonEx_textColor
                        checked: settings ? settings.renameMode === 1 : false
                        onCheckedChanged: {
                            if (checked && settings) {
                                settings.renameMode = 1
                            }
                        }
                    }

                    TextFieldEx {
                        id: searchTextField
                        Layout.preferredWidth: 120
                        text: settings ? settings.searchText : ""
                        placeholderText: "查找"
                        enabled: replaceRadio.checked
                        onTextChanged: {
                            if (settings) {
                                settings.searchText = text
                            }
                        }
                        bgColor: pal.TextFieldEx_bgColor
                        disabledBgColor: pal.TextFieldEx_disabledBgColor
                        textColor: pal.TextFieldEx_textColor
                        disabledTextColor: pal.TextFieldEx_disabledTextColor
                        phColor: pal.TextFieldEx_phColor
                        selColor: pal.TextFieldEx_selColor
                        selTextColor: pal.TextFieldEx_selTextColor
                        borderColor: pal.TextFieldEx_borderColor
                        disabledBorderColor: pal.TextFieldEx_disabledBorderColor
                        focusBorderColor: pal.TextFieldEx_focusBorderColor
                        focusRingColor: pal.TextFieldEx_focusRingColor
                        cursorColor: pal.TextFieldEx_cursorColor
                    }

                    Label {
                        id: arrowLabel
                        text: "→"
                        color: pal.LabelEx_infoText
                    }

                    TextFieldEx {
                        id: replaceTextField
                        Layout.preferredWidth: 120
                        text: settings ? settings.replaceText : ""
                        placeholderText: "替换"
                        enabled: replaceRadio.checked
                        onTextChanged: {
                            if (settings) {
                                settings.replaceText = text
                            }
                        }
                        bgColor: pal.TextFieldEx_bgColor
                        disabledBgColor: pal.TextFieldEx_disabledBgColor
                        textColor: pal.TextFieldEx_textColor
                        disabledTextColor: pal.TextFieldEx_disabledTextColor
                        phColor: pal.TextFieldEx_phColor
                        selColor: pal.TextFieldEx_selColor
                        selTextColor: pal.TextFieldEx_selTextColor
                        borderColor: pal.TextFieldEx_borderColor
                        disabledBorderColor: pal.TextFieldEx_disabledBorderColor
                        focusBorderColor: pal.TextFieldEx_focusBorderColor
                        focusRingColor: pal.TextFieldEx_focusRingColor
                        cursorColor: pal.TextFieldEx_cursorColor
                    }

                    Item { Layout.fillWidth: true }

                    IconButton {
                        iconSource: "qrc:/icons/trash.svg"
                        tooltip: "清空记录"
                        visible: controller ? controller.hasRecords : false
                        normalColor: pal.IconBtnEx_normalColor
                        hoverColor: pal.IconBtnEx_hoverColor
                        pressColor: pal.IconBtnEx_pressColor
                        borderColor: pal.IconBtnEx_borderColor
                        defaultBorderColor: pal.IconBtnEx_defaultBorderColor
                        disabledColor: pal.IconBtnEx_disabledColor
                        disabledBorderColor: pal.IconBtnEx_disabledBorderColor
                        textColor: pal.IconBtnEx_textColor
                        disabledTextColor: pal.IconBtnEx_disabledTextColor
                        shadowColor: pal.IconBtnEx_shadowColor
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
                iconSource: "qrc:/icons/undo.svg"
                tooltip: "批量还原"
                visible: controller ? controller.hasRecords : false
                normalColor: pal.IconBtnEx_normalColor
                hoverColor: pal.IconBtnEx_hoverColor
                pressColor: pal.IconBtnEx_pressColor
                borderColor: pal.IconBtnEx_borderColor
                defaultBorderColor: pal.IconBtnEx_defaultBorderColor
                disabledColor: pal.IconBtnEx_disabledColor
                disabledBorderColor: pal.IconBtnEx_disabledBorderColor
                textColor: pal.IconBtnEx_textColor
                disabledTextColor: pal.IconBtnEx_disabledTextColor
                shadowColor: pal.IconBtnEx_shadowColor
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
                            iconSource: "qrc:/icons/undo.svg"
                            tooltip: modelData.success ? "还原" : "失败项无法还原"
                            enabled: modelData.success
                            normalColor: pal.IconBtnEx_normalColor
                            hoverColor: pal.IconBtnEx_hoverColor
                            pressColor: pal.IconBtnEx_pressColor
                            borderColor: pal.IconBtnEx_borderColor
                            defaultBorderColor: pal.IconBtnEx_defaultBorderColor
                            disabledColor: pal.IconBtnEx_disabledColor
                            disabledBorderColor: pal.IconBtnEx_disabledBorderColor
                            textColor: pal.IconBtnEx_textColor
                            disabledTextColor: pal.IconBtnEx_disabledTextColor
                            shadowColor: pal.IconBtnEx_shadowColor
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
