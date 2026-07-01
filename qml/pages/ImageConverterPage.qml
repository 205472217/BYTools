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
    property QtObject settings: pluginManager.settingsForController(controller)

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

    // 自定义比例编辑中 → 取消预设Rect高亮
    property bool customRatioEditing: false

    // 当前比例是否为预设值
    function isPresetRatio(r) {
        return Math.abs(r - 0.5) < 0.001 || Math.abs(r - 1.5) < 0.001
            || Math.abs(r - 2.0) < 0.001 || Math.abs(r - 4.0) < 0.001
    }

    readonly property bool isSingleMode: settings ? settings.mode === 0 : false

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

    FileDialog {
        id: sourceFileDialog
        title: "选择源文件"
        nameFilters: ["图片文件 (*.png *.jpg *.jpeg *.bmp *.webp *.tiff *.tif *.gif *.ico)", "所有文件 (*)"]
        onAccepted: {
            if (settings) {
                settings.rootPath = decodeURIComponent(selectedFile.toString().replace("file:///", ""))
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
                text: "当前有图片处理任务正在处理中，返回首页将中断执行，是否继续？"
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
                    paletteGroup: "BackCancelBtn"
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: backConfirmDialog.close()
                }

                IconButton {
                    id: goBackBtn
                    text: "返回首页"
                    tooltip: "中断任务并返回首页"
                    paletteGroup: "BackConfirmBtn"
                    implicitWidth: 120
                    implicitHeight: 38
                    onClicked: {
                        if (controller) { controller.reset(); }
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
                    id: pageTitle
                    text: "图片处理"
                    color: pal.LabelEx_titleText
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: pageSubtitle
                    text: "批量转换图片格式、缩放尺寸，支持递归子文件夹"
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

                // ── 第0行：模式切换 ──
                RowLayout {
                    spacing: 0
                    Layout.fillWidth: true

                    RadioButtonEx {
                        id: singleModeRadio
                        implicitWidth: 120
                        text: "单文件模式"
                        paletteGroup: "RadioButtonEx"
                        checked: root.isSingleMode
                        font.pixelSize: 13
                        onCheckedChanged: {
                            if (checked && settings && settings.mode !== 0)
                                settings.mode = 0
                        }
                    }

                    RadioButtonEx {
                        id: batchModeRadio
                        implicitWidth: 120
                        text: "批量处理模式"
                        paletteGroup: "RadioButtonEx"
                        checked: !root.isSingleMode
                        font.pixelSize: 13
                        onCheckedChanged: {
                            if (checked && settings && settings.mode !== 1)
                                settings.mode = 1
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // ── 第1行：源路径 ──
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: srcFolderLbl
                        text: root.isSingleMode ? "源文件" : "源文件夹"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? settings.rootPath : ""
                        readOnly: true
                        placeholderText: root.isSingleMode ? "点击选择文件" : "点击选择文件夹"
                        paletteGroup: "TextFieldEx"
                    }

                    CheckBoxEx {
                        id: recursiveCheck
                        visible: !root.isSingleMode
                        implicitWidth: 110
                        text: "递归子文件夹"
                        paletteGroup: "CheckBoxEx"
                        checked: controller ? settings.recursive : false
                        onCheckedChanged: {
                            if (controller) {
                                settings.recursive = checked
                            }
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: root.isSingleMode ? "选择源文件" : "选择源文件夹"
                        paletteGroup: "IconBtnEx"
                        onClicked: {
                            if (root.isSingleMode)
                                sourceFileDialog.open()
                            else
                                sourceFolderDialog.open()
                        }
                    }
                }

                // ── 第2行：格式转换 + JPG背景色 ──
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    CheckBoxEx {
                        id: convertCheck
                        Layout.preferredWidth: 80
                        text: "格式转换"
                        paletteGroup: "CheckBoxEx"
                        checked: controller ? settings.convertEnabled : true
                        onCheckedChanged: {
                            if (controller) {
                                settings.convertEnabled = checked
                            }
                        }
                    }

                    ComboBoxEx {
                        Layout.preferredWidth: 150
                        model: ["PNG", "JPG", "BMP", "WebP", "TIFF"]
                        currentIndex: controller ? settings.targetFormat : 1
                        enabled: controller ? settings.convertEnabled : false
                        onActivated: {
                            if (controller) {
                                settings.targetFormat = currentIndex
                            }
                        }
                        paletteGroup: "ComboBoxEx"
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
                        enabled: controller ? settings.convertEnabled : false
                    }

                    Slider {
                        id: qualitySlider
                        Layout.fillWidth: true
                        Layout.minimumWidth: 80
                        from: 1
                        to: 100
                        stepSize: 1
                        enabled: controller ? settings.convertEnabled : false
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
                        enabled: controller ? settings.convertEnabled : false
                    }

                    Rectangle {
                        id: qualitySeparator
                        width: 1
                        height: 24
                        color: pal.SurfaceEx_divider
                    }

                    // JPG背景色提示（非JPG时opacity=0但仍占位）
                    Label {
                        id: fillTipLbl
                        text: "PNG转JPG填充背景色"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        opacity: controller && settings.targetFormat === 1 ? 1 : 0
                        enabled: controller && settings.targetFormat === 1
                    }

                    Rectangle {
                        id: whiteSwatch
                        width: 42
                        height: 28
                        radius: 6
                        color: pal.ImageConverterPage_whiteSwatch_color
                        opacity: controller && settings.targetFormat === 1 ? 1 : 0
                        enabled: controller && settings.targetFormat === 1
                        border.width: controller && settings.bgColor === pal.ImageConverterPage_whiteSwatch_color ? 2 : 1
                        border.color: controller && settings.bgColor === pal.ImageConverterPage_whiteSwatch_color ? pal.ImageConverterPage_whiteSwatch_borderColor_active : pal.ImageConverterPage_whiteSwatch_borderColor_normal

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { if (controller) settings.bgColor = pal.ImageConverterPage_whiteSwatch_color }
                        }
                    }

                    Rectangle {
                        id: blackSwatch
                        width: 42
                        height: 28
                        radius: 6
                        color: pal.ImageConverterPage_blackSwatch_color
                        opacity: controller && settings.targetFormat === 1 ? 1 : 0
                        enabled: controller && settings.targetFormat === 1
                        border.width: controller && settings.bgColor === pal.ImageConverterPage_blackSwatch_color ? 2 : 1
                        border.color: controller && settings.bgColor === pal.ImageConverterPage_blackSwatch_color ? pal.ImageConverterPage_blackSwatch_borderColor_active : pal.ImageConverterPage_blackSwatch_borderColor_normal

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { if (controller) settings.bgColor = pal.ImageConverterPage_blackSwatch_color }
                        }
                    }

                    Rectangle {
                        id: customSwatch
                        width: 42
                        height: 28
                        radius: 6
                        color: (controller && settings.bgColor !== pal.ImageConverterPage_whiteSwatch_color && settings.bgColor !== pal.ImageConverterPage_blackSwatch_color) ? settings.bgColor : pal.ImageConverterPage_customSwatch_color
                        opacity: controller && settings.targetFormat === 1 ? 1 : 0
                        enabled: controller && settings.targetFormat === 1
                        border.width: (controller && settings.bgColor !== pal.ImageConverterPage_whiteSwatch_color && settings.bgColor !== pal.ImageConverterPage_blackSwatch_color) ? 2 : 1
                        border.color: (controller && settings.bgColor !== pal.ImageConverterPage_whiteSwatch_color && settings.bgColor !== pal.ImageConverterPage_blackSwatch_color) ? pal.ImageConverterPage_customSwatch_borderColor_active : pal.ImageConverterPage_customSwatch_borderColor_normal

                        Label {
                            id: customSwatchLabel
                            anchors.centerIn: parent
                            text: "自定义"
                            color: (controller && settings.bgColor !== pal.ImageConverterPage_whiteSwatch_color && settings.bgColor !== pal.ImageConverterPage_blackSwatch_color) ? pal.ImageConverterPage_customSwatch_textColor_active : pal.ImageConverterPage_customSwatch_textColor_normal
                            font.pixelSize: 9
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { colorDialog.open() }
                        }
                    }
                }

                // ── 第3行：宽高缩放（按比例 + 指定宽高） ──
                RowLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    CheckBoxEx {
                        id: resizeCheck
                        Layout.preferredWidth: 80
                        text: "宽高缩放"
                        paletteGroup: "CheckBoxEx"
                        checked: controller ? settings.resizeEnabled : false
                        onCheckedChanged: {
                            if (controller) {
                                settings.resizeEnabled = checked
                            }
                        }
                    }

                    // 按比例缩放
                    RadioButtonEx {
                        id: proportionRadio
                        implicitWidth: 100
                        text: "按比例缩放"
                        paletteGroup: "RadioButtonEx"
                        enabled: controller ? settings.resizeEnabled : false
                        checked: controller ? settings.resizeMode === 0 : true
                        onCheckedChanged: {
                            if (checked && controller) {
                                settings.resizeMode = 0
                            }
                        }
                    }

                    // 预设 0.5
                    Rectangle {
                        id: preset05
                        width: 36
                        height: 26
                        radius: 6
                        enabled: controller ? (settings.resizeEnabled && settings.resizeMode === 0) : false
                        color: pal.SurfaceEx_cardBg
                        readonly property bool _active: controller && settings.resizeMode === 0
                            && !customRatioEditing
                            && Math.abs(settings.resizeRatio - 0.5) < 0.001
                        border.width: _active ? 2 : 1
                        border.color: _active ? pal.SurfaceEx_selectedBolderColor : pal.SurfaceEx_unSelectedBolderColor

                        Label {
                            anchors.centerIn: parent
                            text: "0.5"
                            font.pixelSize: 12
                            font.bold: true
                            color: parent.enabled ? pal.LabelEx_valueText : pal.LabelEx_infoText
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            enabled: parent.enabled
                            onClicked: {
                                if (controller) {
                                    settings.resizeMode = 0
                                    settings.resizeRatio = 0.5
                                }
                            }
                        }
                    }

                    // 预设 1.5
                    Rectangle {
                        id: preset15
                        width: 36
                        height: 26
                        radius: 6
                        enabled: controller ? (settings.resizeEnabled && settings.resizeMode === 0) : false
                        color: pal.SurfaceEx_cardBg
                        readonly property bool _active: controller && settings.resizeMode === 0
                            && !customRatioEditing
                            && Math.abs(settings.resizeRatio - 1.5) < 0.001
                        border.width: _active ? 2 : 1
                        border.color: _active ? pal.SurfaceEx_selectedBolderColor : pal.SurfaceEx_unSelectedBolderColor

                        Label {
                            anchors.centerIn: parent
                            text: "1.5"
                            font.pixelSize: 12
                            font.bold: true
                            color: parent.enabled ? pal.LabelEx_valueText : pal.LabelEx_infoText
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            enabled: parent.enabled
                            onClicked: {
                                if (controller) {
                                    settings.resizeMode = 0
                                    settings.resizeRatio = 1.5
                                }
                            }
                        }
                    }

                    // 预设 2
                    Rectangle {
                        id: preset2
                        width: 36
                        height: 26
                        radius: 6
                        enabled: controller ? (settings.resizeEnabled && settings.resizeMode === 0) : false
                        color: pal.SurfaceEx_cardBg
                        readonly property bool _active: controller && settings.resizeMode === 0
                            && !customRatioEditing
                            && Math.abs(settings.resizeRatio - 2.0) < 0.001
                        border.width: _active ? 2 : 1
                        border.color: _active ? pal.SurfaceEx_selectedBolderColor : pal.SurfaceEx_unSelectedBolderColor

                        Label {
                            anchors.centerIn: parent
                            text: "2"
                            font.pixelSize: 12
                            font.bold: true
                            color: parent.enabled ? pal.LabelEx_valueText : pal.LabelEx_infoText
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            enabled: parent.enabled
                            onClicked: {
                                if (controller) {
                                    settings.resizeMode = 0
                                    settings.resizeRatio = 2.0
                                }
                            }
                        }
                    }

                    // 预设 4
                    Rectangle {
                        id: preset4
                        width: 36
                        height: 26
                        radius: 6
                        enabled: controller ? (settings.resizeEnabled && settings.resizeMode === 0) : false
                        color: pal.SurfaceEx_cardBg
                        readonly property bool _active: controller && settings.resizeMode === 0
                            && !customRatioEditing
                            && Math.abs(settings.resizeRatio - 4.0) < 0.001
                        border.width: _active ? 2 : 1
                        border.color: _active ? pal.SurfaceEx_selectedBolderColor : pal.SurfaceEx_unSelectedBolderColor

                        Label {
                            anchors.centerIn: parent
                            text: "4"
                            font.pixelSize: 12
                            font.bold: true
                            color: parent.enabled ? pal.LabelEx_valueText : pal.LabelEx_infoText
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            enabled: parent.enabled
                            onClicked: {
                                if (controller) {
                                    settings.resizeMode = 0
                                    settings.resizeRatio = 4.0
                                }
                            }
                        }
                    }

                    TextFieldEx {
                        id: customRatioEdit
                        Layout.preferredWidth: 80
                        placeholderText: controller ? settings.resizeRatio.toString() : ""
                        paletteGroup: "TextFieldEx"
                        enabled: controller ? (settings.resizeEnabled && settings.resizeMode === 0) : false
                        editType: 2
                        minNumber: 0.1
                        maxNumber: 10
                        highlightOnValid: controller && settings.resizeMode === 0
                            && Math.abs(settings.resizeRatio - 0.5) > 0.001
                            && Math.abs(settings.resizeRatio - 1.5) > 0.001
                            && Math.abs(settings.resizeRatio - 2.0) > 0.001
                            && Math.abs(settings.resizeRatio - 4.0) > 0.001
                        onEditingFinished: {
                            var t = text.trim()
                            if (t === "") {
                                text = settings.resizeRatio.toString()
                                return
                            }
                            var val = parseFloat(t)
                            if (!isNaN(val)) {
                                val = Math.max(0.1, Math.min(10, val))
                                if (val !== parseFloat(t)) text = val.toString()
                                settings.resizeMode = 0
                                settings.resizeRatio = val
                            }
                        }
                        onActiveFocusChanged: {
                            customRatioEditing = activeFocus
                            if (activeFocus && controller) {
                                settings.resizeMode = 0
                            }
                        }
                    }

                    Connections {
                        target: controller ? settings : null
                        function onResizeRatioChanged() {
                            customRatioEdit.text = settings.resizeRatio.toString()
                        }
                    }

                    // 分隔线
                    Rectangle {
                        id: resizeSeparator
                        width: 1
                        height: 24
                        color: pal.SurfaceEx_divider
                        enabled: controller ? settings.resizeEnabled : false
                    }

                    // 指定宽高缩放
                    RadioButtonEx {
                        id: fixedRadio
                        implicitWidth: 120
                        text: "指定宽高缩放"
                        paletteGroup: "RadioButtonEx"
                        enabled: controller ? settings.resizeEnabled : false
                        checked: controller ? settings.resizeMode === 1 : true
                        onCheckedChanged: {
                            if (checked && controller) {
                                settings.resizeMode = 1
                            }
                        }
                    }

                    TextFieldEx {
                        id: widthEdit
                        Layout.preferredWidth: 60
                        placeholderText: "宽"
                        paletteGroup: "TextFieldEx"
                        editType: 1
                        minNumber: 1
                        maxNumber: 9999
                        enabled: controller ? (settings.resizeEnabled && settings.resizeMode === 1) : false
                        text: controller ? settings.resizeWidth.toString() : ""
                        onEditingFinished: {
                            var val = parseInt(text)
                            if (!isNaN(val) && controller) {
                                settings.resizeWidth = val
                            }
                        }
                        onActiveFocusChanged: {
                            if (activeFocus && controller && settings.resizeMode !== 1) {
                                settings.resizeMode = 1
                            }
                        }
                    }

                    Label {
                        id: multiplySign
                        text: "×"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 15
                        font.bold: true
                        enabled: controller ? (settings.resizeEnabled && settings.resizeMode === 1) : false
                    }

                    TextFieldEx {
                        id: heightEdit
                        Layout.preferredWidth: 60
                        placeholderText: "高"
                        paletteGroup: "TextFieldEx"
                        editType: 1
                        minNumber: 1
                        maxNumber: 9999
                        enabled: controller ? (settings.resizeEnabled && settings.resizeMode === 1) : false
                        text: controller ? settings.resizeHeight.toString() : ""
                        onEditingFinished: {
                            var val = parseInt(text)
                            if (!isNaN(val) && controller) {
                                settings.resizeHeight = val
                            }
                        }
                        onActiveFocusChanged: {
                            if (activeFocus && controller && settings.resizeMode !== 1) {
                                settings.resizeMode = 1
                            }
                        }
                    }

                    Label {
                        id: pxLbl
                        text: "像素"
                        color: pal.LabelEx_infoText
                        font.pixelSize: 12
                        enabled: controller ? (settings.resizeEnabled && settings.resizeMode === 1) : false
                    }

                    Item { Layout.fillWidth: true }
                }

                // ── 第4行： 输出方式 ──
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: outputModeLbl
                        text: "输出方式"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 76
                    }

                    RadioButtonEx {
                        id: replaceRadio
                        implicitWidth: 120
                        text: "替换原文件"
                        paletteGroup: "RadioButtonEx"
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
                        paletteGroup: "RadioButtonEx"
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
                        paletteGroup: "TextFieldEx"
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择输出目录"
                        enabled: newDirRadio.checked
                        paletteGroup: "IconBtnEx"
                        onClicked: outputFolderDialog.open()
                    }

                    Item { Layout.fillWidth: true }

                    IconButton {
                        id: execBtn
                        implicitWidth: 150
                        text: "开始处理"
                        iconSource: "qrc:/icons/play.svg"
                        tooltip: "开始转换图片格式"
                        paletteGroup: "ImageConverterPage_execBtn"
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
                paletteGroup: "IconBtnEx"
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
                            color: modelData.formatTag && modelData.formatTag === "PNG" ? pal.LabelEx_PNG_BgRect :
                                   modelData.formatTag && modelData.formatTag === "JPG" ? pal.LabelEx_JPG_BgRect :
                                   modelData.formatTag && modelData.formatTag === "BMP" ? pal.LabelEx_BMP_BgRect :
                                   modelData.formatTag && modelData.formatTag === "WEBP" ? pal.LabelEx_WEBP_BgRect :
                                   modelData.formatTag && modelData.formatTag === "TIFF" ? pal.LabelEx_TIFF_BgRect :
                                   modelData.formatTag && modelData.formatTag === "GIF" ? pal.LabelEx_GIF_BgRect : pal.LabelEx_other_BgRect

                            Label {
                                anchors.centerIn: parent
                                text: modelData.formatTag
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.formatTag && modelData.formatTag === "PNG" ? pal.LabelEx_PNG_Text :
                                       modelData.formatTag && modelData.formatTag === "JPG" ? pal.LabelEx_JPG_Text :
                                       modelData.formatTag && modelData.formatTag === "BMP" ? pal.LabelEx_BMP_Text :
                                       modelData.formatTag && modelData.formatTag === "WEBP" ? pal.LabelEx_WEBP_Text :
                                       modelData.formatTag && modelData.formatTag === "TIFF" ? pal.LabelEx_TIFF_Text :
                                       modelData.formatTag && modelData.formatTag === "GIF" ? pal.LabelEx_GIF_Text : pal.LabelEx_other_Text
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


