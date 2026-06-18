import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

Pane {
    id: root

    property var pal: themeManager.palette

    signal backRequested
    signal openSettings

    property var controller: null

    // Real-time log model — auto-scrolls to end when new items arrive
    ListModel {
        id: logModel
        onCountChanged: {
            Qt.callLater(function() {
                logListView.positionViewAtEnd();
            });
        }
    }

    // Elapsed time tracker — updates last log entry with step + running time
    Timer {
        id: elapsedTimer
        interval: 1000
        running: controller ? controller.isProcessing : false
        repeat: true

        property int seconds: 0            // per-step seconds
        property int totalSeconds: 0       // total seconds across all steps
        property int elapsedIndex: -1
        property string lastStep: ""
        property string elapsedString: "00:00:00"           // per-step display
        property string totalElapsedString: "00:00:00"      // total display
        property string finalElapsedString: ""

        function pad(n) {
            return n < 10 ? "0" + n : "" + n;
        }

        function formatTime(s) {
            return pad(Math.floor(s / 3600)) + ":"
                 + pad(Math.floor((s % 3600) / 60)) + ":"
                 + pad(s % 60);
        }

        function resetStep() {
            seconds = 0;
            elapsedIndex = -1;
            elapsedString = "00:00:00";
            lastStep = controller ? controller.currentStep : "";
            if (controller && controller.isProcessing && lastStep.length > 0) {
                logModel.append({ "text": lastStep + "...(已耗时 00:00:00)" });
                elapsedIndex = logModel.count - 1;
            }
        }

        function resetAll() {
            seconds = 0;
            totalSeconds = 0;
            elapsedIndex = -1;
            elapsedString = "00:00:00";
            totalElapsedString = "00:00:00";
            lastStep = "";
            finalElapsedString = "";
        }

        onTriggered: {
            if (!controller || !controller.isProcessing) {
                resetAll();
                return;
            }

            // If step changed, start fresh per-step counter, keep total
            var step = controller.currentStep;
            if (step.length === 0) return;  // not in a step yet
            if (step !== lastStep) {
                resetStep();
                return;
            }

            seconds++;
            totalSeconds++;
            elapsedString = formatTime(seconds);
            totalElapsedString = formatTime(totalSeconds);
            var text = step + "...(已耗时 " + elapsedString + ")";

            if (elapsedIndex >= 0 && elapsedIndex < logModel.count) {
                logModel.setProperty(elapsedIndex, "text", text);
            } else {
                logModel.append({ "text": text });
                elapsedIndex = logModel.count - 1;
            }
        }
    }

    Connections {
        target: controller
        function onLogMessage(message) {
            if (message.length === 0) return;
            logModel.append({ "text": message });
        }
    }

    // 处理开始 → 重置计时 | 处理结束 → 记录最终耗时
    Connections {
        target: controller
        function onIsProcessingChanged() {
            if (!controller) return;
            if (controller.isProcessing) {
                // 新任务开始 → 重置所有计时（清除上次的 totalSeconds，重新累积）
                elapsedTimer.resetAll();
            } else if (elapsedTimer.totalSeconds > 0) {
                // 任务结束（完成/中止）→ 记录最终总耗时，供 header 显示
                elapsedTimer.finalElapsedString = elapsedTimer.formatTime(elapsedTimer.totalSeconds);
            }
        }
    }

    // Step 变更 → 立即开始新步骤计时（在 completion 消息之后）
    Connections {
        target: controller
        function onCurrentStepChanged() {
            if (controller && controller.isProcessing) {
                elapsedTimer.resetStep();
            }
        }
    }

    padding: 0
    background: Rectangle {
        id: pageBg
        color: pal.VideoSubtitlePage_pageBg_color
    }

    FolderDialog {
        id: inputFolderDialog
        title: "选择输入路径"
        onAccepted: {
            if (controller) {
                controller.inputPath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""));
            }
        }
    }

    FileDialog {
        id: inputFileDialog
        title: "选择视频文件"
        nameFilters: ["视频文件 (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.ts)", "所有文件 (*)"]
        onAccepted: {
            if (controller) {
                controller.inputPath = decodeURIComponent(selectedFile.toString().replace("file:///", ""));
            }
        }
    }

    FolderDialog {
        id: outputFolderDialog
        title: "选择输出目录"
        onAccepted: {
            if (controller) {
                controller.outputDir = decodeURIComponent(selectedFolder.toString().replace("file:///", ""));
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
                id: confirmLabel
                text: "当前有任务正在处理中，返回首页将中断执行，是否继续？"
                color: pal.VideoSubtitlePage_confirmLabel_color
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
                    normalColor: pal.VideoSubtitlePage_cancelBtn_normalColor
                    hoverColor: pal.VideoSubtitlePage_cancelBtn_hoverColor
                    borderColor: pal.VideoSubtitlePage_cancelBtn_borderColor
                    textColor: pal.VideoSubtitlePage_cancelBtn_textColor
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: {
                        backConfirmDialog.close();
                    }
                }

                IconButton {
                    id: backToHomeBtn
                    text: "返回首页"
                    tooltip: "中断任务并返回首页"
                    normalColor: pal.VideoSubtitlePage_backToHomeBtn_normalColor
                    hoverColor: pal.VideoSubtitlePage_backToHomeBtn_hoverColor
                    borderColor: pal.VideoSubtitlePage_backToHomeBtn_borderColor
                    textColor: pal.VideoSubtitlePage_backToHomeBtn_textColor
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
        spacing: 18

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            IconButton {
                iconSource: "qrc:/icons/arrow-left.svg"
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
                    text: "视频字幕翻译"
                    color: pal.VideoSubtitlePage_pageTitle_color
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: pageSubtitle
                    text: "可自定义处理步骤：分离音频 → 语音识别 → 翻译 → 烧录字幕"
                    color: pal.VideoSubtitlePage_pageSubtitle_color
                    font.pixelSize: 14
                }
            }

            Item {
                Layout.fillWidth: true
            }
            
            IconButton {
                iconSource: "qrc:/icons/settings.svg"
                tooltip: "设置"
                onClicked: root.openSettings()
            }
        }

        // Settings card
        Rectangle {
            id: settingsCard
            Layout.fillWidth: true
            Layout.fillHeight: false
            implicitHeight: settingsColumn.implicitHeight + 36
            radius: 10
            color: pal.VideoSubtitlePage_settingsCard_color
            border.color: pal.VideoSubtitlePage_settingsCard_borderColor
            border.width: 1

            // 面板阴影
            Rectangle {
                id: settingsCardShadow
                anchors.fill: parent
                anchors.margins: -2
                radius: 12
                color: pal.VideoSubtitlePage_settingsCardShadow_color
                opacity: 0.04
                z: -1
            }

            ColumnLayout {
                id: settingsColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                // Row 1: Input mode
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: inputModeLabel
                        text: "输入模式"
                        color: pal.VideoSubtitlePage_inputModeLabel_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RadioButton {
                        text: "单个视频"
                        checked: controller ? controller.inputMode === 0 : true
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.inputMode = 0;
                                controller.inputPath = "";
                            }
                        }
                    }

                    RadioButton {
                        text: "文件夹批量"
                        checked: controller ? controller.inputMode === 1 : false
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.inputMode = 1;
                                controller.inputPath = "";
                            }
                        }
                    }

                    CheckBox {
                        text: "递归子文件夹"
                        checked: controller ? controller.recursive : false
                        visible: controller ? controller.inputMode === 1 : false
                        onCheckedChanged: {
                            if (controller)
                                controller.recursive = checked;
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }

                // Row 2: Input path
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: inputPathLabel
                        text: "输入路径"
                        color: pal.VideoSubtitlePage_inputPathLabel_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.inputPath : ""
                        readOnly: true
                        placeholderText: "选择视频文件或文件夹"
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择文件"
                        onClicked: {
                            if (controller && controller.inputMode === 0) {
                                inputFileDialog.open();
                            } else {
                                inputFolderDialog.open();
                            }
                        }
                    }
                }

                // Row 3: Step selection
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: stepLabel
                        text: "处理步骤"
                        color: pal.VideoSubtitlePage_stepLabel_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 4

                        CheckBox {
                            id: stepAudio
                            text: "1. 分离音频"
                            checked: controller ? controller.enableAudioExtraction : true
                            opacity: 1.0
                            onCheckedChanged: {
                                if (controller)
                                    controller.enableAudioExtraction = checked;
                                if (!checked && controller) {
                                    controller.enableTranscribe = false;
                                    controller.enableTranslate = false;
                                    controller.enableBurnSubtitle = false;
                                }
                            }
                        }

                        CheckBox {
                            id: stepTranscribe
                            text: "2. 语音识别(音频→SRT)"
                            checked: controller ? controller.enableTranscribe : true
                            enabled: stepAudio.checked
                            opacity: 1.0
                            onCheckedChanged: {
                                if (controller)
                                    controller.enableTranscribe = checked;
                                if (!checked && controller) {
                                    controller.enableTranslate = false;
                                    controller.enableBurnSubtitle = false;
                                }
                            }
                            ToolTip {
                                text: "请先勾选「1. 分离音频」"
                                visible: parent.hovered && !parent.enabled
                                delay: 400
                            }
                        }

                        CheckBox {
                            id: stepTranslate
                            text: "3. 翻译字幕"
                            checked: controller ? controller.enableTranslate : true
                            enabled: stepAudio.checked && stepTranscribe.checked
                            opacity: 1.0
                            onCheckedChanged: {
                                if (controller)
                                    controller.enableTranslate = checked;
                            }
                            ToolTip {
                                text: "请先勾选「1. 分离音频」和「2. 语音识别」"
                                visible: parent.hovered && !parent.enabled
                                delay: 400
                            }
                        }

                        CheckBox {
                            id: stepBurn
                            text: "4. 烧录字幕"
                            checked: controller ? controller.enableBurnSubtitle : true
                            enabled: stepAudio.checked && stepTranscribe.checked
                            opacity: 1.0
                            onCheckedChanged: {
                                if (controller)
                                    controller.enableBurnSubtitle = checked;
                            }
                            ToolTip {
                                text: "请先勾选「1. 分离音频」和「2. 语音识别」"
                                visible: parent.hovered && !parent.enabled
                                delay: 400
                            }
                        }
                    }
                }

                // Row 4: Language settings (only relevant when translate is enabled)
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: langLabel
                        text: "语种转换"
                        color: pal.VideoSubtitlePage_langLabel_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    Label {
                        id: sourceLangLabel
                        text: "源语言"
                        color: pal.VideoSubtitlePage_sourceLangLabel_color
                        font.pixelSize: 13
                        font.bold: true
                    }

                    ComboBoxEx {
                        enabled: stepTranslate.checked
                        Layout.preferredWidth: 120
                        model: ["自动检测", "英文", "中文", "日文", "韩文", "俄语"]
                        currentIndex: {
                            if (!controller)
                                return 0;
                            switch (controller.sourceLanguage) {
                            case "auto": return 0;
                            case "en":   return 1;
                            case "zh":   return 2;
                            case "ja":   return 3;
                            case "ko":   return 4;
                            case "ru":   return 5;
                            default:     return 0;
                            }
                        }
                        onActivated: {
                            if (!controller) return;
                            var langs = ["auto", "en", "zh", "ja", "ko", "ru"];
                            controller.sourceLanguage = langs[currentIndex];
                        }
                    }

                    Label {
                        id: targetLangLabel
                        text: "目标语言"
                        color: pal.VideoSubtitlePage_targetLangLabel_color
                        font.pixelSize: 13
                        font.bold: true
                    }

                    ComboBoxEx {
                        enabled: stepTranslate.checked
                        Layout.preferredWidth: 100
                        model: ["中文", "英文", "日文", "韩文"]
                        currentIndex: {
                            if (!controller) return 0;
                            switch (controller.targetLanguage) {
                            case "zh": return 0;
                            case "en": return 1;
                            case "ja": return 2;
                            case "ko": return 3;
                            default:   return 0;
                            }
                        }
                        onActivated: {
                            if (!controller) return;
                            var langs = ["zh", "en", "ja", "ko"];
                            controller.targetLanguage = langs[currentIndex];
                        }
                    }

                    CheckBox {
                        id: musicCheck
                        text: "背景音乐(翻译+烧录)"
                        checked: controller ? controller.translateMusic : false
                        font.pixelSize: 11
                        Layout.fillWidth: true
                        enabled: stepTranslate.checked
                        onCheckedChanged: {
                            if (controller)
                                controller.translateMusic = checked;
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }

                // Row 5: Output
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: outputLabel
                        text: "输出方式"
                        color: pal.VideoSubtitlePage_outputLabel_color
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RadioButton {
                        text: "同目录"
                        checked: controller ? controller.outputMode === 0 : true
                        onCheckedChanged: {
                            if (checked && controller)
                                controller.outputMode = 0;
                        }
                    }

                    RadioButton {
                        text: "指定目录"
                        checked: controller ? controller.outputMode === 1 : false
                        onCheckedChanged: {
                            if (checked && controller)
                                controller.outputMode = 1;
                        }
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.outputDir : ""
                        placeholderText: "输出目录"
                        readOnly: true
                        enabled: controller ? controller.outputMode === 1 : false
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择输出目录"
                        enabled: controller ? controller.outputMode === 1 : false
                        onClicked: outputFolderDialog.open()
                    }

                    IconButton {
                        id: startBtn
                        implicitWidth: 130
                        implicitHeight: 40
                        text: controller && controller.isProcessing ? "中止处理" : "开始处理"
                        iconSource: controller && controller.isProcessing ? "" : "qrc:/icons/play.svg"
                        tooltip: controller && controller.isProcessing ? "中止处理" : "开始处理"
                        normalColor: controller && controller.isProcessing ? pal.VideoSubtitlePage_startBtn_normalColor_active : pal.VideoSubtitlePage_startBtn_normalColor_normal
                        hoverColor: controller && controller.isProcessing ? pal.VideoSubtitlePage_startBtn_hoverColor_active : pal.VideoSubtitlePage_startBtn_hoverColor_normal
                        borderColor: controller && controller.isProcessing ? pal.VideoSubtitlePage_startBtn_borderColor_active : pal.VideoSubtitlePage_startBtn_borderColor_normal
                        textColor: pal.VideoSubtitlePage_startBtn_textColor
                        enabled: true
                        onClicked: {
                            if (controller) {
                                if (controller.isProcessing) {
                                    controller.cancel();
                                } else {
                                    controller.execute();
                                }
                            }
                        }
                    }
                }
            }
        }

        // Merged status + progress bar (compact)
        Rectangle {
            id: statusBar
            Layout.fillWidth: true
            Layout.preferredHeight: controller && controller.isProcessing ? 34 : 26
            radius: 6
            color: controller && controller.statusMessage ? pal.VideoSubtitlePage_statusBar_color_active : "transparent"
            visible: controller ? (controller.isProcessing || controller.statusMessage.length > 0) : false

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.topMargin: 4
                anchors.bottomMargin: 4
                spacing: 2

                Label {
                    id: statusMessage
                    Layout.fillWidth: true
                    text: controller ? controller.statusMessage : ""
                    color: pal.VideoSubtitlePage_statusMessage_color
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: controller ? controller.isProcessing : false

                    Rectangle {
                        id: progressTrack
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                        Layout.alignment: Qt.AlignVCenter
                        radius: 2
                        color: pal.VideoSubtitlePage_progressTrack_color

                        Rectangle {
                            id: progressFill
                            width: parent.width * (controller ? controller.progress : 0)
                            height: parent.height
                            radius: 2
                            color: pal.VideoSubtitlePage_progressFill_color
                        }
                    }

                    Label {
                        id: progressPercent
                        text: controller ? Math.round(controller.progress * 100) + "%" : ""
                        color: pal.VideoSubtitlePage_progressPercent_color
                        font.pixelSize: 11
                        font.bold: true
                    }
                }
            }
        }

        // Real-time log output
        Rectangle {
            id: logCard
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: pal.VideoSubtitlePage_logCard_color
            border.color: pal.VideoSubtitlePage_logCard_borderColor
            border.width: 1
            clip: true

            Rectangle {
                id: logCardShadow
                anchors.fill: parent
                anchors.margins: -2
                radius: 12
                color: pal.VideoSubtitlePage_logCardShadow_color
                opacity: 0.04
                z: -1
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header row
                Rectangle {
                    id: logHeaderBg
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: pal.VideoSubtitlePage_logHeaderBg_color
                    radius: 0

                    Rectangle {
                        id: logHeaderDivider
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: pal.VideoSubtitlePage_logHeaderDivider_color
                    }

                    Label {
                        id: logHeaderTitle
                        anchors.left: parent.left
                        anchors.leftMargin: 18
                        anchors.verticalCenter: parent.verticalCenter
                        text: controller && controller.isProcessing
                              ? "任务执行中（已耗时 " + elapsedTimer.totalElapsedString + "）"
                              : (elapsedTimer.finalElapsedString.length > 0 && logModel.count > 0
                                 ? "任务耗时（" + elapsedTimer.finalElapsedString + "）"
                                 : "")
                        color: controller && controller.isProcessing ? pal.VideoSubtitlePage_logHeaderTitle_color_active : pal.VideoSubtitlePage_logHeaderTitle_color_normal
                        font.pixelSize: 12
                        font.bold: true
                    }

                    IconButton {
                        anchors.right: parent.right
                        iconSource: "qrc:/icons/trash.svg"
                        tooltip: "清空日志"
                        visible: logModel.count > 0 || elapsedTimer.finalElapsedString.length > 0
                        onClicked: {
                            logModel.clear();
                            elapsedTimer.finalElapsedString = "";
                            if (controller)
                                controller.clearRecords();
                        }
                    }
                }

                // Log entries
                ListView {
                    id: logListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 200
                    model: logModel
                    clip: true
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AlwaysOn
                    }
                    spacing: 0

                    delegate: Rectangle {
                        id: logDelegate
                        width: logListView.width
                        height: Math.max(28, logText.implicitHeight + 10)
                        color: index % 2 === 0 ? pal.VideoSubtitlePage_logDelegate_color_even : pal.VideoSubtitlePage_logDelegate_color_odd

                        Rectangle {
                            id: logDelegateDivider
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: pal.VideoSubtitlePage_logDelegateDivider_color
                        }

                        Label {
                            id: logText
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 18
                            anchors.rightMargin: 18
                            text: model.text
                            font.pixelSize: 12
                            font.family: "Consolas, 'Courier New', monospace"
                            font.bold: {
                                var c = model.text.charAt(0);
                                return c === '=' || c === '?';
                            }
                            color: {
                                var c = model.text.charAt(0);
                                if (c === '?') return pal.VideoSubtitlePage_logText_color_error;
                                if (c === '?') return pal.VideoSubtitlePage_logText_color_success;
                                if (c === '→') return pal.VideoSubtitlePage_logText_color_info;
                                if (c === '=') return pal.VideoSubtitlePage_logText_color_heading;
                                return pal.VideoSubtitlePage_logText_color_default;
                            }
                            wrapMode: Text.Wrap
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    // Empty state
                    Column {
                        anchors.centerIn: parent
                        spacing: 8
                        visible: logModel.count === 0

                        Label {
                            id: emptyTitle
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "暂无输出"
                            color: pal.VideoSubtitlePage_emptyTitle_color
                            font.pixelSize: 15
                        }
                        Label {
                            id: emptySubtitle
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "开始处理后这里将显示实时日志"
                            color: pal.VideoSubtitlePage_emptySubtitle_color
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}


