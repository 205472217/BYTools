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

    Component.onDestruction: {
        if (controller && typeof controller.reset === 'function') {
            controller.reset();
        }
    }

    // Navigate to settings when controller detects missing configuration
    Connections {
        target: controller
        function onSettingsRequired() {
            root.openSettings();
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
                onClicked: root.backRequested()
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
                    text: "提取音频 → 语音识别 → 翻译 → 烧录字幕"
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
            Layout.preferredHeight: 330
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
                            if (checked && controller)
                                controller.inputMode = 0;
                        }
                    }

                    RadioButton {
                        text: "文件夹批量"
                        checked: controller ? controller.inputMode === 1 : false
                        onCheckedChanged: {
                            if (checked && controller)
                                controller.inputMode = 1;
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

                // Row 3: Source language → Target language
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "源语言"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    ComboBoxEx {
                        Layout.preferredWidth: 120
                        model: ["自动检测", "英文", "中文", "日文", "韩文"]
                        currentIndex: {
                            if (!controller)
                                return 0;
                            switch (controller.sourceLanguage) {
                            case "auto":
                                return 0;
                            case "en":
                                return 1;
                            case "zh":
                                return 2;
                            case "ja":
                                return 3;
                            case "ko":
                                return 4;
                            default:
                                return 0;
                            }
                        }
                        onActivated: {
                            if (!controller)
                                return;
                            var langs = ["auto", "en", "zh", "ja", "ko"];
                            controller.sourceLanguage = langs[currentIndex];
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 24
                        color: "#e2e8f0"
                    }

                    Label {
                        text: "目标语言"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    ComboBoxEx {
                        Layout.preferredWidth: 100
                        model: ["中文", "英文", "日文", "韩文"]
                        currentIndex: {
                            if (!controller)
                                return 0;
                            switch (controller.targetLanguage) {
                            case "zh":
                                return 0;
                            case "en":
                                return 1;
                            case "ja":
                                return 2;
                            case "ko":
                                return 3;
                            default:
                                return 0;
                            }
                        }
                        onActivated: {
                            if (!controller)
                                return;
                            var langs = ["zh", "en", "ja", "ko"];
                            controller.targetLanguage = langs[currentIndex];
                        }
                    }
                }

                // Row 4: Subtitle style
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "字幕样式"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    ComboBoxEx {
                        Layout.preferredWidth: 120
                        model: ["默认白色", "黄色描边", "自定义"]
                        currentIndex: controller ? controller.subtitleStyle : 0
                        onActivated: {
                            if (controller)
                                controller.subtitleStyle = currentIndex;
                        }
                    }

                    CheckBox {
                        text: "双语字幕"
                        checked: controller ? controller.bilingual : true
                        onCheckedChanged: {
                            if (controller)
                                controller.bilingual = checked;
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
                }

                // Row 6: Other options + action buttons
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "其他"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    CheckBox {
                        text: "保留 SRT 文件"
                        checked: controller ? controller.keepOriginalSrt : true
                        onCheckedChanged: {
                            if (controller)
                                controller.keepOriginalSrt = checked;
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 8

                        IconButton {
                            iconSource: "qrc:/icons/trash.svg"
                            tooltip: "清空记录"
                            visible: controller ? controller.hasRecords : false
                            onClicked: {
                                if (controller)
                                    controller.clearRecords();
                            }
                        }

                        IconButton {
                            implicitWidth: 120
                            implicitHeight: 40
                            text: "开始处理"
                            iconSource: "qrc:/icons/play.svg"
                            tooltip: "开始处理"
                            normalColor: "#2563eb"
                            hoverColor: "#1d4ed8"
                            borderColor: "#1d4ed8"
                            enabled: controller ? !controller.isProcessing : false
                            onClicked: {
                                if (controller)
                                    controller.execute();
                            }
                        }
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            Layout.fillWidth: true
            height: statusText.implicitHeight + 12
            radius: 6
            color: controller && controller.statusMessage ? "#eff6ff" : "transparent"
            visible: controller ? controller.statusMessage.length > 0 : false

            Label {
                id: statusText
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 12
                text: controller ? controller.statusMessage : ""
                color: "#2563eb"
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }

        // Progress bar (visible when processing)
        Rectangle {
            Layout.fillWidth: true
            height: 40
            radius: 6
            color: "#eff6ff"
            visible: controller ? controller.isProcessing : false

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                Label {
                    text: controller ? controller.currentStep + "..." : ""
                    color: "#2563eb"
                    font.pixelSize: 12
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 4
                    radius: 2
                    color: "#e2e8f0"

                    Rectangle {
                        width: parent.width * (controller ? controller.progress : 0)
                        height: parent.height
                        radius: 2
                        color: "#2563eb"
                    }
                }
            }
        }

        // Results table
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "#ffffff"
            border.color: "#e5e9f0"
            border.width: 1
            clip: true

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
                anchors.fill: parent
                anchors.margins: 2
                spacing: 0

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

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 20

                        Label {
                            width: 60
                            anchors.verticalCenter: parent.verticalCenter
                            text: "状态"
                            color: "#64748b"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Label {
                            width: (parent.width - 60 - 220) / 2
                            anchors.verticalCenter: parent.verticalCenter
                            text: "原文件名"
                            color: "#64748b"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Label {
                            width: (parent.width - 60 - 220) / 2
                            anchors.verticalCenter: parent.verticalCenter
                            text: "输出文件名"
                            color: "#64748b"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Label {
                            width: 220
                            anchors.verticalCenter: parent.verticalCenter
                            text: "状态信息"
                            color: "#64748b"
                            font.pixelSize: 12
                            font.bold: true
                        }
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
                        width: recordsListView.width
                        height: 56
                        color: index % 2 === 0 ? "#ffffff" : "#fafbfc"

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: "#f1f5f9"
                        }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 18
                            anchors.rightMargin: 20

                            Label {
                                width: 60
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.success ? "✅" : "❌"
                                font.pixelSize: 16
                            }

                            Label {
                                width: (parent.width - 60 - 220) / 2
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.originalName
                                color: "#334155"
                                font.pixelSize: 13
                                elide: Text.ElideMiddle
                            }

                            Label {
                                width: (parent.width - 60 - 220) / 2
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.outputName
                                color: modelData.success ? "#059669" : "#dc2626"
                                font.pixelSize: 13
                                elide: Text.ElideMiddle
                            }

                            Label {
                                width: 220
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.status
                                color: modelData.success ? "#059669" : "#dc2626"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // Empty state
                    Column {
                        anchors.centerIn: parent
                        spacing: 8
                        visible: controller ? recordsListView.count === 0 : true

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "暂无处理记录"
                            color: "#94a3b8"
                            font.pixelSize: 15
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "设置参数后点击开始处理"
                            color: "#c7d2e0"
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}
