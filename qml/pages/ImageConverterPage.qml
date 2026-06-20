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
        color: pal.ImageConverterPage_bgRect_color
    }

    FolderDialog {
        id: sourceFolderDialog
        title: "选择源文件夹"
        onAccepted: {
            if (controller) {
                controller.rootPath = selectedFolder
            }
        }
    }

    FolderDialog {
        id: outputFolderDialog
        title: "选择输出目录"
        onAccepted: {
            if (controller) {
                controller.outputDir = selectedFolder
            }
        }
    }

    ColorDialog {
        id: colorDialog
        selectedColor: controller ? controller.bgColor : pal.ImageConverterPage_colorDialog_defaultColor
        onAccepted: {
            if (controller) {
                controller.bgColor = selectedColor.toString()
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
                text: "当前有图片格式转换任务正在处理中，返回首页将中断执行，是否继续？"
                color: pal.ImageConverterPage_msgLabel_color
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
                    color: pal.ImageConverterPage_pageTitle_color
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: pageSubtitle
                    text: "批量转换图片格式，支持递归子文件夹"
                    color: pal.ImageConverterPage_pageSubtitle_color
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
            color: pal.ImageConverterPage_settingsCard_color
            border.color: pal.ImageConverterPage_settingsCard_borderColor
            border.width: 1

            Rectangle {
                id: settingsShadow
                anchors.fill: parent
                anchors.margins: -2
                radius: 12
                color: pal.ImageConverterPage_settingsShadow_color
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
                        id: srcFolderLbl
                        text: "源文件夹"
                        color: pal.ImageConverterPage_srcFolderLbl_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.rootPath : ""
                        readOnly: true
                        placeholderText: "点击选择文件"
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
                        onClicked: sourceFolderDialog.open()
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: targetFormatLbl
                        text: "目标格式"
                        color: pal.ImageConverterPage_targetFormatLbl_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    ComboBoxEx {
                        Layout.preferredWidth: 150
                        model: ["PNG", "JPG", "BMP", "WebP", "TIFF"]
                        currentIndex: controller ? controller.targetFormat : 1
                        onActivated: {
                            if (controller) {
                                controller.targetFormat = currentIndex
                            }
                        }
                    }

                    Rectangle {
                        id: fmtSeparator
                        width: 1
                        height: 24
                        color: pal.ImageConverterPage_fmtSeparator_color
                    }

                    Label {
                        id: qualityLbl
                        text: "质量"
                        color: pal.ImageConverterPage_qualityLbl_color
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
                            if (controller) value = controller.quality
                        }
                        onValueChanged: {
                            if (pressed && controller) {
                                controller.quality = Math.round(value)
                            }
                        }
                    }

                    Label {
                        id: qualityValLbl
                        text: (controller ? controller.quality : 85)
                        color: pal.ImageConverterPage_qualityValLbl_color
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
                    visible: controller ? controller.targetFormat === 1 : false

                    Label {
                        id: bgColorLbl
                        text: "JPG背景色"
                        color: pal.ImageConverterPage_bgColorLbl_color
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
                            border.width: controller && controller.bgColor === "#ffffff" ? 2 : 1
                            border.color: controller && controller.bgColor === "#ffffff" ? pal.ImageConverterPage_whiteSwatch_borderColor_active : pal.ImageConverterPage_whiteSwatch_borderColor_normal

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (controller) controller.bgColor = "#ffffff" }
                            }
                        }

                        Rectangle {
                            id: blackSwatch
                            width: 42
                            height: 28
                            radius: 6
                            color: pal.ImageConverterPage_blackSwatch_color
                            border.width: controller && controller.bgColor === "#000000" ? 2 : 1
                            border.color: controller && controller.bgColor === "#000000" ? pal.ImageConverterPage_blackSwatch_borderColor_active : pal.ImageConverterPage_blackSwatch_borderColor_normal

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (controller) controller.bgColor = "#000000" }
                            }
                        }

                        Rectangle {
                            id: customSwatch
                            width: 42
                            height: 28
                            radius: 6
                            color: pal.ImageConverterPage_customSwatch_color
                            border.width: (controller && controller.bgColor !== "#ffffff" && controller.bgColor !== "#000000") ? 2 : 1
                            border.color: (controller && controller.bgColor !== "#ffffff" && controller.bgColor !== "#000000") ? pal.ImageConverterPage_customSwatch_borderColor_active : pal.ImageConverterPage_customSwatch_borderColor_normal

                            Label {
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
                        color: pal.ImageConverterPage_fmtSeparator_color
                    }

                    Label {
                        id: fillTipLbl
                        text: "PNG转JPG时填充透明区域"
                        color: pal.ImageConverterPage_fillTipLbl_color
                        font.pixelSize: 12
                    }

                    Rectangle {
                        id: customSwatchPreview
                        width: 100
                        height: 28
                        radius: 6
                        color: controller ? controller.bgColor : pal.ImageConverterPage_customSwatch_color

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
                        color: pal.ImageConverterPage_outputModeLbl_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RadioButton {
                        id: replaceRadio
                        text: "替换原文件"
                        checked: controller ? controller.outputMode === 0 : true
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.outputMode = 0
                            }
                        }
                    }

                    RadioButton {
                        id: newDirRadio
                        text: "输出到新目录"
                        checked: controller ? controller.outputMode === 1 : false
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.outputMode = 1
                            }
                        }
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.outputDir : ""
                        placeholderText: controller ? (controller.rootPath ? controller.rootPath + "_converted" : "自动在源目录后添加 _converted") : ""
                        readOnly: false
                        enabled: newDirRadio.checked
                        onTextChanged: {
                            if (controller) {
                                controller.outputDir = text
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

        // 状态栏
        Rectangle {
            id: statusBar
            Layout.fillWidth: true
            height: statusText.implicitHeight + 12
            radius: 6
            color: pal.ImageConverterPage_statusBar_color
            visible: controller ? controller.statusMessage.length > 0 : false

            Label {
                id: statusText
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 12
                text: controller ? controller.statusMessage : ""
                color: pal.ImageConverterPage_statusTextLbl_color
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }

        // 结果列表
        Rectangle {
            id: resultCard
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: pal.ImageConverterPage_resultCard_color
            border.color: pal.ImageConverterPage_resultCard_borderColor
            border.width: 1
            clip: true

            Rectangle {
                id: resultShadow
                anchors.fill: parent
                anchors.margins: -2
                radius: 12
                color: pal.ImageConverterPage_resultShadow_color
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
                    color: pal.ImageConverterPage_headerRow_color

                    Rectangle {
                        id: headerBorder
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: pal.ImageConverterPage_headerBorder_color
                    }

                    Label {
                        id: fmtHeaderLbl
                        x: root.typeColumnX
                        width: root.typeColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "格式"
                        color: pal.ImageConverterPage_fmtHeaderLbl_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: origHeaderLbl
                        x: root.originalColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "原名称"
                        color: pal.ImageConverterPage_origHeaderLbl_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: newHeaderLbl
                        x: root.newColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "新名称"
                        color: pal.ImageConverterPage_newHeaderLbl_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: statusHeaderLbl
                        x: root.statusColumnX
                        width: root.statusColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "状态"
                        color: pal.ImageConverterPage_statusHeaderLbl_color
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        id: actionHeaderLbl
                        x: root.actionColumnX
                        width: root.actionColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "操作"
                        color: pal.ImageConverterPage_actionHeaderLbl_color
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
                        color: rowMouseArea.containsMouse ? pal.ImageConverterPage_rowDelegate_color_hover :
                               index % 2 === 0 ? pal.ImageConverterPage_rowDelegate_color_even : pal.ImageConverterPage_rowDelegate_color_odd

                        Rectangle {
                            id: rowBorder
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: pal.ImageConverterPage_rowBorder_color
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
                            color: pal.ImageConverterPage_origNameLbl_color
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        Label {
                            id: newNameLbl
                            x: root.newColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: modelData.status === "已跳过" ? modelData.originalName : modelData.newName
                            color: modelData.status === "已跳过" ? pal.ImageConverterPage_newNameLbl_color_skipped : pal.ImageConverterPage_newNameLbl_color_done
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
                            color: modelData.status === "已转换" ? pal.ImageConverterPage_statusBadge_color_converted :
                                   modelData.status === "已跳过" ? pal.ImageConverterPage_statusBadge_color_skipped : pal.ImageConverterPage_statusBadge_color_failed

                            Label {
                                id: statusTagLbl
                                anchors.centerIn: parent
                                text: modelData.status
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.status === "已转换" ? pal.ImageConverterPage_statusTagLbl_color_converted :
                                       modelData.status === "已跳过" ? pal.ImageConverterPage_statusTagLbl_color_skipped : pal.ImageConverterPage_statusTagLbl_color_failed
                                elide: Text.ElideRight
                            }
                        }

                        Label {
                            id: pathLbl
                            x: root.originalColumnX
                            y: 40
                            width: root.statusColumnX - root.originalColumnX - root.columnGap
                            text: modelData.success ? modelData.newPath : modelData.originalPath
                            color: pal.ImageConverterPage_pathLbl_color
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
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
                            color: pal.ImageConverterPage_emptyTitleLbl_color
                            font.pixelSize: 15
                        }
                        Label {
                            id: emptySubtitleLbl
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "设置参数后点击执行按钮开始"
                            color: pal.ImageConverterPage_emptySubtitleLbl_color
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}


