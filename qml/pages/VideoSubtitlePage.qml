import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

Pane {
    id: root

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
            // Keep only: ✓ ✗ = - 开头的重要消息，其余中间消息由计时器处理
            if (message.length === 0) return;
            var c = message.charAt(0);
            if (c === '→' || c === '跳' || c === '生') return;  // 跳过进度/跳过/生成
            if (c !== '✓' && c !== '✗' && c !== '=' && c !== '-') return;
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

    Component.onDestruction: {
        if (controller && typeof controller.reset === 'function') {
            controller.reset();
        }
    }

    padding: 0
    background: Rectangle {
        color: "#f4f6f9"
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
                text: "当前有任务正在处理中，返回首页将中断执行，是否继续？"
                color: "#334155"
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
                    normalColor: "#e2e8f0"
                    hoverColor: "#cbd5e1"
                    borderColor: "#cbd5e1"
                    textColor: "#475569"
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: {
                        backConfirmDialog.close();
                    }
                }

                IconButton {
                    text: "返回首页"
                    tooltip: "中断任务并返回首页"
                    normalColor: "#dc2626"
                    hoverColor: "#b91c1c"
                    borderColor: "#b91c1c"
                    textColor: "#ffffff"
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
                    text: "视频字幕翻译"
                    color: "#111827"
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: "可自定义处理步骤：分离音频 → 语音识别 → 翻译 → 烧录字幕"
                    color: "#64748b"
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
            Layout.fillWidth: true
            Layout.fillHeight: false
            implicitHeight: settingsColumn.implicitHeight + 36
            radius: 10
            color: "#ffffff"
            border.color: "#e5e9f0"
            border.width: 1

            // 面板阴影
            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                radius: 12
                color: "#1e3a5f"
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
                        text: "输入模式"
                        color: "#475569"
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
                        text: "输入路径"
                        color: "#475569"
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
                        text: "处理步骤"
                        color: "#475569"
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
                        text: "翻译选项"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    Label {
                        text: "源语言"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    ComboBoxEx {
                        enabled: stepTranslate.checked
                        Layout.preferredWidth: 120
                        model: ["自动检测", "英文", "中文", "日文", "韩文"]
                        currentIndex: {
                            if (!controller)
                                return 0;
                            switch (controller.sourceLanguage) {
                            case "auto": return 0;
                            case "en":   return 1;
                            case "zh":   return 2;
                            case "ja":   return 3;
                            case "ko":   return 4;
                            default:     return 0;
                            }
                        }
                        onActivated: {
                            if (!controller) return;
                            var langs = ["auto", "en", "zh", "ja", "ko"];
                            controller.sourceLanguage = langs[currentIndex];
                        }
                    }

                    Label {
                        text: "目标语言"
                        color: "#475569"
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

                    Item {
                        Layout.fillWidth: true
                    }
                }

                // Row 5: Output
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "输出方式"
                        color: "#475569"
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
                        implicitWidth: 130
                        implicitHeight: 40
                        text: controller && controller.isProcessing ? "中止处理" : "开始处理"
                        iconSource: controller && controller.isProcessing ? "" : "qrc:/icons/play.svg"
                        tooltip: controller && controller.isProcessing ? "中止处理" : "开始处理"
                        normalColor: controller && controller.isProcessing ? "#dc2626" : "#2563eb"
                        hoverColor: controller && controller.isProcessing ? "#b91c1c" : "#1d4ed8"
                        borderColor: controller && controller.isProcessing ? "#b91c1c" : "#1d4ed8"
                        textColor: "#ffffff"
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
            Layout.fillWidth: true
            Layout.preferredHeight: controller && controller.isProcessing ? 34 : 26
            radius: 6
            color: controller && controller.statusMessage ? "#eff6ff" : "transparent"
            visible: controller ? (controller.isProcessing || controller.statusMessage.length > 0) : false

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.topMargin: 4
                anchors.bottomMargin: 4
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: controller ? controller.statusMessage : ""
                    color: "#475569"
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: controller ? controller.isProcessing : false

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                        Layout.alignment: Qt.AlignVCenter
                        radius: 2
                        color: "#e2e8f0"

                        Rectangle {
                            width: parent.width * (controller ? controller.progress : 0)
                            height: parent.height
                            radius: 2
                            color: "#2563eb"
                        }
                    }

                    Label {
                        text: controller ? Math.round(controller.progress * 100) + "%" : ""
                        color: "#2563eb"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }
            }
        }

        // Real-time log output
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "#ffffff"
            border.color: "#e5e9f0"
            border.width: 1
            clip: true

            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                radius: 12
                color: "#1e3a5f"
                opacity: 0.04
                z: -1
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header row
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: "#f8fafc"
                    radius: 0

                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: "#e8ecf2"
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 18
                        anchors.verticalCenter: parent.verticalCenter
                        text: controller && controller.isProcessing
                              ? "任务执行中（已耗时 " + elapsedTimer.totalElapsedString + "）"
                              : (elapsedTimer.finalElapsedString.length > 0 && logModel.count > 0
                                 ? "任务耗时（" + elapsedTimer.finalElapsedString + "）"
                                 : "")
                        color: controller && controller.isProcessing ? "#2563eb" : "#64748b"
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
                        width: logListView.width
                        height: Math.max(28, logText.implicitHeight + 10)
                        color: index % 2 === 0 ? "#ffffff" : "#fafbfc"

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: "#f1f5f9"
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
                                return c === '=' || c === '✗';
                            }
                            color: {
                                var c = model.text.charAt(0);
                                if (c === '✗') return "#dc2626";
                                if (c === '✓') return "#059669";
                                if (c === '→') return "#2563eb";
                                if (c === '=') return "#1e293b";
                                return "#475569";
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
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "暂无输出"
                            color: "#94a3b8"
                            font.pixelSize: 15
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "开始处理后这里将显示实时日志"
                            color: "#c7d2e0"
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}
