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
    property QtObject settings: pluginManager.getPluginSettings("image-converter")

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
        id: bgRect
        color: pal.SurfaceEx_pageBg
    }

    FolderDialog {
        id: sourceFolderDialog
        title: "选择源文件夹"
        onAccepted: {
            if (settings) {
                settings.rootPath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""))
            }
        }
    }

    FolderDialog {
        id: outputFolderDialog
        title: "选择输出目录"
        onAccepted: {
            if (settings) {
                settings.outputDir = decodeURIComponent(selectedFolder.toString().replace("file:///", ""))
            }
        }
    }

    ColorDialog {
        id: colorDialog
        selectedColor: settings ? settings.bgColor : pal.ImageConverterPage_colorDialog_defaultColor
        onAccepted: {
            if (settings) {
                settings.bgColor = selectedColor.toString()
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
                id: msgLabel
                text: "当前有图片格式转换任务正在处理中，返回首页将中断执行，是否继续？"
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
                    normalColor: pal.ImageConverterPage_cancelBtn_normalColor
                    hoverColor: pal.ImageConverterPage_cancelBtn_hoverColor
                    borderColor: pal.ImageConverterPage_cancelBtn_borderColor
                    textColor: pal.ImageConverterPage_cancelBtn_textColor
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: backConfirmDialog.close()
                }

                IconButton {
                    id: goBackBtn
                    text: "返回首页"
                    tooltip: "中断任务并返回首页"
                    normalColor: pal.ImageConverterPage_goBackBtn_normalColor
                    hoverColor: pal.ImageConverterPage_goBackBtn_hoverColor
                    borderColor: pal.ImageConverterPage_goBackBtn_borderColor
                    textColor: pal.ImageConverterPage_goBackBtn_textColor
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
                    id: pageTitle
                    text: "图片格式转换"
                    color: pal.LabelEx_titleText
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: pageSubtitle
                    text: "批量转换图片格式，支持递归子文件夹"
                    color: pal.LabelEx_subtitleText
                    font.pixelSize: 14
                }
            }
        }

        Rectangle {
            id: settingsCard
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
                        id: srcFolderLbl
                        text: "源文件夹"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? settings.rootPath : ""
                        readOnly: true
                        placeholderText: "点击选择文件"
                    }

                    CheckBoxEx {
                        id: recursiveCheck
                        implicitWidth: 110
                        text: "递归子文件夹"
                        textColor: pal.CheckBoxEx_textColor
                        checked: controller ? settings.recursive : false
                        onCheckedChanged: {
                            if (controller) {
                                settings.recursive = checked
                            }
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择源文件夹"
                        onClicked: sourceFolderDialog.open()
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: targetFormatLbl
                        text: "目标格式"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    ComboBoxEx {
                        Layout.preferredWidth: 150
                        model: ["PNG", "JPG", "BMP", "WebP", "TIFF"]
                        currentIndex: controller ? settings.targetFormat : 1
                        onActivated: {
                            if (controller) {
                                settings.targetFormat = currentIndex
                            }
                        }
                    }

                    Rectangle {
                        id: fmtSeparator
                        width: 1
                        height: 24
                        color: pal.SurfaceEx_divider
                    }

                    Label {
                        id: qualityLbl
                        text: "质量"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Slider {
                        id: qualitySlider
                        Layout.fillWidth: true
                        Layout.minimumWidth: 80
                        from: 1
                        to: 100
                        stepSize: 1
                        Component.onCompleted: {
                            if (controller) value = settings.quality
                        }
                        onValueChanged: {
                            if (pressed && controller) {
                                settings.quality = Math.round(value)
                            }
                        }
                    }

                    Label {
                        id: qualityValLbl
                        text: (controller ? settings.quality : 85)
                        color: pal.LabelEx_valueText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 28
                        horizontalAlignment: Text.AlignRight
                    }
                }

                // JPG背景色（仅目标格式为JPG时显示）
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true
                    visible: controller ? settings.targetFormat === 1 : false

                    Label {
                        id: bgColorLbl
                        text: "JPG背景色"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RowLayout {
                        spacing: 12

                        Rectangle {
                            id: whiteSwatch
                            width: 42
                            height: 28
                            radius: 6
                            color: pal.ImageConverterPage_whiteSwatch_color
                            border.width: controller && settings.bgColor === "#ffffff" ? 2 : 1
                            border.color: controller && settings.bgColor === "#ffffff" ? pal.ImageConverterPage_whiteSwatch_borderColor_active : pal.ImageConverterPage_whiteSwatch_borderColor_normal

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (controller) settings.bgColor = "#ffffff" }
                            }
                        }

                        Rectangle {
                            id: blackSwatch
                            width: 42
                            height: 28
                            radius: 6
                            color: pal.ImageConverterPage_blackSwatch_color
                            border.width: controller && settings.bgColor === "#000000" ? 2 : 1
                            border.color: controller && settings.bgColor === "#000000" ? pal.ImageConverterPage_blackSwatch_borderColor_active : pal.ImageConverterPage_blackSwatch_borderColor_normal

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (controller) settings.bgColor = "#000000" }
                            }
                        }

                        Rectangle {
                            id: customSwatch
                            width: 42
                            height: 28
                            radius: 6
                            color: pal.ImageConverterPage_customSwatch_color
                            border.width: (controller && settings.bgColor !== "#ffffff" && settings.bgColor !== "#000000") ? 2 : 1
                            border.color: (controller && settings.bgColor !== "#ffffff" && settings.bgColor !== "#000000") ? pal.ImageConverterPage_customSwatch_borderColor_active : pal.ImageConverterPage_customSwatch_borderColor_normal

                            Label {
                                id: customSwatchLabel
                                anchors.centerIn: parent
                                text: "自定义"
                                color: pal.ImageConverterPage_customSwatch_textColor_normal
                                font.pixelSize: 9
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { colorDialog.open() }
                            }
                        }
                    }

                    Rectangle {
                        id: bgColorSeparator
                        width: 1
                        height: 24
                        color: pal.SurfaceEx_divider
                    }

                    Label {
                        id: fillTipLbl
                        text: "PNG转JPG时填充透明区域"
                        color: pal.LabelEx_infoText
                        font.pixelSize: 12
                    }

                    Rectangle {
                        id: customSwatchPreview
                        width: 100
                        height: 28
                        radius: 6
                        color: controller ? settings.bgColor : pal.ImageConverterPage_customSwatch_color

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: colorDialog.open()
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: outputModeLbl
                        text: "输出方式"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RadioButtonEx {
                        id: replaceRadio
                        implicitWidth: 120
                        text: "替换原文件"
                        textColor: pal.RadioButtonEx_textColor
                        checked: controller ? settings.outputMode === 0 : true
                        onCheckedChanged: {
                            if (checked && controller) {
                                settings.outputMode = 0
                            }
                        }
                    }

                    RadioButtonEx {
                        id: newDirRadio
                        implicitWidth: 120
                        text: "输出到新目录"
                        textColor: pal.RadioButtonEx_textColor
                        checked: controller ? settings.outputMode === 1 : false
                        onCheckedChanged: {
                            if (checked && controller) {
                                settings.outputMode = 1
                            }
                        }
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? settings.outputDir : ""
                        placeholderText: controller ? (settings.rootPath ? settings.rootPath + "_converted" : "自动在源目录后添加 _converted") : ""
                        readOnly: true
                        enabled: newDirRadio.checked
                        onTextChanged: {
                            if (controller) {
                                settings.outputDir = text
                            }
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择输出目录"
                        enabled: newDirRadio.checked
                        onClicked: outputFolderDialog.open()
                    }

                    Item { Layout.fillWidth: true }

                    IconButton {
                        id: execBtn
                        implicitWidth: 150
                        text: "开始处理"
                        iconSource: "qrc:/icons/play.svg"
                        tooltip: "开始转换图片格式"
                        normalColor: pal.ImageConverterPage_execBtn_normalColor
                        hoverColor: pal.ImageConverterPage_execBtn_hoverColor
                        borderColor: pal.ImageConverterPage_execBtn_borderColor
                        onClicked: {
                            if (controller) {
                                controller.executeConvert()
                            }
                        }
                    }
                }
            }
        }

        // 状态栏 + 批量还原
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            spacing: 12
            visible: true

            Rectangle {
                id: statusBar
                Layout.fillWidth: true
                height: statusText.implicitHeight + 12
                radius: 6
                color: pal.SurfaceEx_statusBar

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
                onClicked: {
                    if (controller) {
                        controller.restoreAllRecords()
                    }
                }
            }
        }

        // 结果列表
        Rectangle {
            id: resultCard
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

                    Rectangle {
                        id: headerBorder
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: pal.SurfaceEx_headerDivider
                    }

                    Label {
                        id: fmtHeaderLbl
                        x: root.typeColumnX
                        width: root.typeColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "格式"
                        color: pal.LabelEx_headerText
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: origHeaderLbl
                        x: root.originalColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "原名称"
                        color: pal.LabelEx_headerText
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: newHeaderLbl
                        x: root.newColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "新名称"
                        color: pal.LabelEx_headerText
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: statusHeaderLbl
                        x: root.statusColumnX
                        width: root.statusColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "状态"
                        color: pal.LabelEx_headerText
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: actionHeaderLbl
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

                        Rectangle {
                            id: rowBorder
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: pal.SurfaceEx_rowDivider
                        }

                        // 格式标签
                        Rectangle {
                            x: root.typeColumnX
                            width: root.typeColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: modelData.formatTag && modelData.formatTag === "PNG" ? "#dbeafe" :
                                   modelData.formatTag && modelData.formatTag === "JPG" ? "#d1fae5" :
                                   modelData.formatTag && modelData.formatTag === "BMP" ? "#f3e8ff" :
                                   modelData.formatTag && modelData.formatTag === "WEBP" ? "#fef3c7" :
                                   modelData.formatTag && modelData.formatTag === "TIFF" ? "#fce7f3" :
                                   modelData.formatTag && modelData.formatTag === "GIF" ? "#fef3c7" : "#f1f5f9"

                            Label {
                                anchors.centerIn: parent
                                text: modelData.formatTag
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.formatTag && modelData.formatTag === "PNG" ? "#2563eb" :
                                       modelData.formatTag && modelData.formatTag === "JPG" ? "#0f766e" :
                                       modelData.formatTag && modelData.formatTag === "BMP" ? "#7c3aed" :
                                       modelData.formatTag && modelData.formatTag === "WEBP" ? "#b45309" :
                                       modelData.formatTag && modelData.formatTag === "TIFF" ? "#be185d" :
                                       modelData.formatTag && modelData.formatTag === "GIF" ? "#b45309" : "#64748b"
                            }
                        }

                        Label {
                            id: origNameLbl
                            x: root.originalColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: modelData.originalName
                            color: pal.LabelEx_valueText
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        Label {
                            id: newNameLbl
                            x: root.newColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: modelData.status === "已跳过" ? modelData.originalName : modelData.newName
                            color: modelData.status === "已跳过" ? pal.LabelEx_infoText : pal.LabelEx_successText
                            font.bold: modelData.status !== "已跳过"
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        // 状态标签
                        Rectangle {
                            id: statusBadge
                            x: root.statusColumnX
                            width: root.statusColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: modelData.status === "已转换" ? pal.StatusBadgeEx_bg_success :
                                   modelData.status === "已跳过" ? pal.StatusBadgeEx_bg_idle : pal.StatusBadgeEx_bg_error

                            Label {
                                id: statusTagLbl
                                anchors.centerIn: parent
                                text: modelData.status
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.status === "已转换" ? pal.LabelEx_statusText :
                                       modelData.status === "已跳过" ? pal.LabelEx_infoText : pal.LabelEx_errorText
                                elide: Text.ElideRight
                            }
                        }

                        Label {
                            id: pathLbl
                            x: root.originalColumnX
                            y: 40
                            width: root.statusColumnX - root.originalColumnX - root.columnGap
                            text: modelData.success ? modelData.newPath : modelData.originalPath
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
                            id: emptyTitleLbl
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "暂无转换记录"
                            color: pal.LabelEx_infoText
                            font.pixelSize: 15
                        }
                        Label {
                            id: emptySubtitleLbl
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "设置参数后点击执行按钮开始"
                            color: pal.LabelEx_infoText
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}


