import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

Pane {
    id: root

    signal backRequested

    property var controller: null

    padding: 0
    background: Rectangle {
        color: "#f4f6f9"
    }

    // ── Dialogs ──
    FolderDialog {
        id: subtitleDownloadFolderDialog
        title: "选择字幕下载路径"
        onAccepted: {
            if (controller)
                controller.subtitleDownloadPath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""));
        }
    }

    FolderDialog {
        id: videoSourceFolderDialog
        title: "选择原视频路径"
        onAccepted: {
            if (controller)
                controller.videoSourcePath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""));
        }
    }

    FolderDialog {
        id: mergedOutputFolderDialog
        title: "选择合成视频路径"
        onAccepted: {
            if (controller)
                controller.mergedOutputPath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""));
        }
    }

    FileDialog {
        id: ffmpegFileDialog
        title: "选择 ffmpeg.exe"
        nameFilters: ["ffmpeg (ffmpeg.exe)", "All Files (*)"]
        onAccepted: {
            if (controller)
                controller.ffmpegPath = decodeURIComponent(selectedFile.toString().replace("file:///", ""));
        }
    }

    // ── Log state ──
    property string _lastLogLine: ""

    Connections {
        target: controller
        function onLogMessage(message) {
            if (message.length === 0)
                return;
            _lastLogLine = message;
        }
    }

    Component.onDestruction: {
        if (controller && typeof controller.reset === 'function') {
            controller.reset();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 14

        // ═══════════════════════════════════════
        // Header
        // ═══════════════════════════════════════
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
                    text: "自定义视频字幕"
                    color: "#111827"
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: "从网站下载字幕，匹配视频并合成，替换原视频 — 一站式完成"
                    color: "#64748b"
                    font.pixelSize: 14
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        // ═══════════════════════════════════════
        // Path Configuration (3 rows)
        // ═══════════════════════════════════════
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

            // 路径配置
            ColumnLayout {
                id: settingsColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                // Row 1: 字幕下载路径
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "字幕下载"
                        color: "#475569"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        implicitHeight: 26
                        text: controller ? controller.subtitleDownloadPath : ""
                        readOnly: true
                        placeholderText: "字幕文件下载后保存的目录路径"
                        font.pixelSize: 11
                    }

                    IconButton {
                        implicitWidth: 26
                        implicitHeight: 26
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择下载路径"
                        onClicked: subtitleDownloadFolderDialog.open()
                    }
                }

                // Row 2: 原视频路径
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "原视频"
                        color: "#475569"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        implicitHeight: 26
                        text: controller ? controller.videoSourcePath : ""
                        readOnly: true
                        placeholderText: "存放原视频的目录路径"
                        font.pixelSize: 11
                    }

                    CheckBox {
                        text: "递归"
                        checked: controller ? controller.recursive : false
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        onCheckedChanged: {
                            if (controller)
                                controller.recursive = checked;
                        }
                    }

                    IconButton {
                        implicitWidth: 26
                        implicitHeight: 26
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择视频目录"
                        onClicked: videoSourceFolderDialog.open()
                    }
                }
                
                // Row 3: 合成输出路径
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "合成输出"
                        color: "#475569"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        implicitHeight: 26
                        text: controller ? controller.mergedOutputPath : ""
                        readOnly: true
                        placeholderText: "合成后视频文件的输出目录路径"
                        font.pixelSize: 11
                    }

                    IconButton {
                        implicitWidth: 26
                        implicitHeight: 26
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择输出目录"
                        onClicked: mergedOutputFolderDialog.open()
                    }
                }

                // Row 4: FFmpeg 路径
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "FFmpeg"
                        color: "#475569"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        implicitHeight: 26
                        text: controller ? controller.ffmpegPath : ""
                        readOnly: true
                        placeholderText: "选择 ffmpeg.exe 路径（用于合成视频+字幕）"
                        font.pixelSize: 11
                    }

                    IconButton {
                        implicitWidth: 26
                        implicitHeight: 26
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择 ffmpeg.exe"
                        onClicked: ffmpegFileDialog.open()
                    }
                }
            }
        }

        // ═══════════════════════════════════════
        // Merged status + progress bar (compact)
        // ═══════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            radius: 6
            color: (controller && controller.statusMessage) ? "#eff6ff" : "transparent"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.topMargin: 4
                anchors.bottomMargin: 4
                spacing: 2

                // Row 1: real-time log + current file
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: controller && controller.isProcessing
                              ? (_lastLogLine.length > 0 ? _lastLogLine : "")
                        : (_lastLogLine.length > 0
                                 ? _lastLogLine
                                 : (controller && controller.statusMessage.length > 0
                                    ? controller.statusMessage : ""))
                        color: "#475569"
                        font.pixelSize: 11
                        font.family: "Consolas, 'Courier New', monospace"
                        elide: Text.ElideRight
                    }

                    Label {
                        visible: controller && controller.isProcessing
                                 && controller.currentFile.length > 0
                        text: "[" + controller.currentFile + "]"
                        color: "#2563EB"
                        font.pixelSize: 11
                        font.family: "Consolas, 'Courier New', monospace"
                        font.bold: true
                    }
                }

                // Row 2: progress（步骤3耗时→走进度条，步骤2/4→走总进度）
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
                        visible: controller && controller.currentStep === "合成视频+字幕"

                        Rectangle {
                            width: parent.width * (controller ? controller.progress : 0)
                            height: parent.height
                            radius: 2
                            color: "#2563eb"
                        }
                    }

                    Label {
                        text: controller && controller.currentStep === "合成视频+字幕"
                              ? Math.round(controller.progress * 100) + "%"
                              : (controller.totalCount > 0
                                 ? controller.processedCount + "/" + controller.totalCount
                                 : "")
                        color: "#2563eb"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }
            }
        }

        // ═══════════════════════════════════════
        // Main content: Left (browser 3/4) + Right (steps 1/4)
        // ═══════════════════════════════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // ── Left Panel: Web Browser ─────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: "#f8fafc"
                border.color: "#e2e8f0"
                border.width: 1
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 0

                    // Step 1 header
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 8

                            Label {
                                text: "步骤1：下载字幕"
                                color: "#111827"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "下载的文件将保存到字幕下载路径"
                                color: "#94a3b8"
                                font.pixelSize: 10
                            }
                        }
                    }

                    // separator 
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#e2e8f0"
                    }

                    // Browser area: Loader for WebEnginePage + fallback
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Loader {
                            id: browserLoader
                            anchors.fill: parent
                            source: Qt.resolvedUrl("../components/WebEnginePage.qml")
                            visible: status === Loader.Ready

                            onLoaded: {
                                if (item) {
                                    item.downloadPath = controller ? controller.subtitleDownloadPath : "";
                                    item.downloadRequested.connect(function (url, fileName) {
                                        if (controller) {
                                            controller.logMessage("✓ 开始下载: " + fileName);
                                            controller.logMessage(" 来源: " + url);
                                        }
                                    });
                                }
                            }
                        }

                        // Fallback when QtWebEngine is not available
                        Rectangle {
                            anchors.fill: parent
                            visible: browserLoader.status !== Loader.Ready
                            color: "#f8fafc"

                            Column {
                                anchors.centerIn: parent
                                spacing: 14

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "内嵌浏览器不可用"
                                    color: "#94a3b8"
                                    font.pixelSize: 16
                                    font.bold: true
                                }

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请在下方输入字幕网站URL，点击按钮在系统浏览器中打开"
                                    color: "#c7d2e0"
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 350
                                    wrapMode: Text.WordWrap
                                }

                                IconButton {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: fallbackUrlInput.text.length > 0 ? "在浏览器中打开" : "请输入URL"
                                    tooltip: "打开系统浏览器访问字幕网站"
                                    normalColor: "#2563eb"
                                    hoverColor: "#1d4ed8"
                                    borderColor: "#1d4ed8"
                                    textColor: "#ffffff"
                                    enabled: fallbackUrlInput.text.trim().length > 0
                                    onClicked: {
                                        var url = fallbackUrlInput.text.trim();
                                        if (url.indexOf("://") < 0)
                                            url = "https://" + url;
                                        Qt.openUrlExternally(url);
                                    }
                                }

                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 300
                                    height: 30
                                    radius: 4
                                    color: "#ffffff"
                                    border.color: "#e2e8f0"
                                    border.width: 1

                                    TextField {
                                        id: fallbackUrlInput
                                        anchors.fill: parent
                                        anchors.leftMargin: 0
                                        anchors.rightMargin: 0
                                        topPadding: 4
                                        bottomPadding: 4
                                        background: Item {}
                                        verticalAlignment: TextInput.AlignVCenter
                                        font.pixelSize: 12
                                        color: "#334155"
                                        placeholderText: "输入字幕网站URL..."
                                        selectByMouse: true
                                        onAccepted: {
                                            var url = text.trim();
                                            if (url.length === 0)
                                                return;
                                            if (url.indexOf("://") < 0)
                                                url = "https://" + url;
                                            Qt.openUrlExternally(url);
                                        }
                                    }
                                }

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "下载字幕文件后点击右侧「步骤2」继续处理"
                                    color: "#94a3b8"
                                    font.pixelSize: 11
                                }
                            }
                        }
                    }
                }
            }

            // ── Right Panel: Steps 2-4 ─────────────────
            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                color: "transparent"
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 0
                    spacing: 10

                    // ── Step 2 ──
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: 8
                        color: "#f8fafc"
                        border.color: "#e2e8f0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5

                            RowLayout {
                                spacing: 5

                                Label {
                                    text: "步骤2：匹配并移动字幕"
                                    color: "#111827"
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.fillWidth: true
                                }

                                BusyIndicator {
                                    width: 40
                                    height: 40
                                    implicitWidth: 20
                                    implicitHeight: 20
                                    running: controller && controller.currentStep === "匹配并移动字幕"
                                    visible: running
                                }
                            }

                            Label {
                                text: "自动匹配下载的字幕与视频，重命名并移动到视频目录"
                                color: "#64748b"
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            IconButton {
                                Layout.fillWidth: true
                                implicitHeight: 28
                                text: "执行"
                                tooltip: "匹配并移动字幕"
                                normalColor: controller && controller.isProcessing ? "#94a3b8" : "#3b82f6"
                                hoverColor: "#2563eb"
                                borderColor: "#2563eb"
                                textColor: "#ffffff"
                                enabled: !controller || !controller.isProcessing
                                onClicked: {
                                    if (controller)
                                        controller.matchAndMoveSubtitles();
                                }
                            }
                        }
                    }

                    // ── Step 3 ──
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: 8
                        color: "#f8fafc"
                        border.color: "#e2e8f0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5

                            RowLayout {
                                spacing: 5

                                Label {
                                    text: "步骤3：合成视频+字幕"
                                    color: "#111827"
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.fillWidth: true
                                }

                                BusyIndicator {
                                    width: 40
                                    height: 40
                                    implicitWidth: 20
                                    implicitHeight: 20
                                    running: controller && controller.currentStep === "合成视频+字幕"
                                    visible: running
                                }
                            }

                            Label {
                                text: "将字幕嵌入视频，生成带字幕的新视频文件"
                                color: "#64748b"
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                // 空闲状态：执行按钮
                                CheckBox {
                                    text: "GPU加速"
                                    checked: controller ? controller.gpuAccel : false
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    enabled: !controller || !controller.isProcessing
                                    onCheckedChanged: {
                                        if (controller)
                                            controller.gpuAccel = checked;
                                    }
                                    visible: !controller || !controller.isProcessing
                                }

                                IconButton {
                                    Layout.preferredWidth: 72
                                    implicitHeight: 28
                                    text: "执行"
                                    tooltip: "合成视频+字幕"
                                    normalColor: "#8b5cf6"
                                    hoverColor: "#7c3aed"
                                    borderColor: "#7c3aed"
                                    textColor: "#ffffff"
                                    enabled: !controller || !controller.isProcessing
                                    visible: !controller || !controller.isProcessing
                                    onClicked: {
                                        if (controller)
                                            controller.mergeSubtitleToVideo();
                                    }
                                }

                                // 运行状态：停止控制
                                IconButton {
                                    Layout.preferredWidth: 84
                                    implicitHeight: 28
                                    text: "当前完成停止"
                                    tooltip: "处理完当前视频后停止，不再继续下一个"
                                    normalColor: "#f59e0b"
                                    hoverColor: "#d97706"
                                    borderColor: "#d97706"
                                    textColor: "#ffffff"
                                    enabled: controller ? controller.isProcessing : false
                                    visible: controller ? controller.isProcessing : false
                                    onClicked: {
                                        if (controller)
                                            controller.requestStopAfterCurrent();
                                    }
                                }

                                IconButton {
                                    Layout.preferredWidth: 72
                                    implicitHeight: 28
                                    text: "立即停止"
                                    tooltip: "强制终止当前合成任务"
                                    normalColor: "#dc2626"
                                    hoverColor: "#b91c1c"
                                    borderColor: "#b91c1c"
                                    textColor: "#ffffff"
                                    enabled: controller ? controller.isProcessing : false
                                    visible: controller ? controller.isProcessing : false
                                    onClicked: {
                                        if (controller)
                                            controller.cancel();
                                    }
                                }
                            }
                        }
                    }

                    // ── Step 4 ──
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: 8
                        color: "#f8fafc"
                        border.color: "#e2e8f0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5

                            RowLayout {
                                spacing: 5

                                Label {
                                    text: "步骤4：匹配+替换原视频"
                                    color: "#111827"
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.fillWidth: true
                                }

                                BusyIndicator {
                                    width: 40
                                    height: 40
                                    implicitWidth: 20
                                    implicitHeight: 20
                                    running: controller && controller.currentStep === "替换原视频"
                                    visible: running
                                }
                            }

                            Label {
                                text: "用合成后的视频替换原文件，并清理同名字幕"
                                color: "#64748b"
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                CheckBox {
                                    text: "备份原文件"
                                    checked: controller ? controller.backupOriginal : false
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    enabled: !controller || !controller.isProcessing
                                    onCheckedChanged: {
                                        if (controller)
                                            controller.backupOriginal = checked;
                                    }
                                }

                                IconButton {
                                    Layout.preferredWidth: 72
                                    implicitHeight: 28
                                    text: "执行"
                                    tooltip: "替换原视频"
                                    normalColor: "#dc2626"
                                    hoverColor: "#b91c1c"
                                    borderColor: "#b91c1c"
                                    textColor: "#ffffff"
                                    enabled: !controller || !controller.isProcessing
                                    onClicked: {
                                        if (controller)
                                            controller.replaceOriginalVideo();
                                    }
                                }

                                IconButton {
                                    Layout.preferredWidth: 72
                                    implicitHeight: 28
                                    text: "取消替换"
                                    tooltip: "强制终止替换操作"
                                    normalColor: "#dc2626"
                                    hoverColor: "#b91c1c"
                                    borderColor: "#b91c1c"
                                    textColor: "#ffffff"
                                    enabled: controller ? controller.isProcessing : false
                                    visible: controller ? controller.isProcessing : false
                                    onClicked: {
                                        if (controller)
                                            controller.cancel();
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }
}
