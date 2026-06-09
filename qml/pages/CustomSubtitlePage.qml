import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

Pane {
    id: root

    signal backRequested

    property var controller: null
    property var _logEntries: [] // full log kept internally

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

    // ── Log state ──
    property string _lastLogLine: ""

    Connections {
        target: controller
        function onLogMessage(message) {
            if (message.length === 0)
                return;
            _logEntries.push(message);
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
            }
        }

        // ═══════════════════════════════════════
        // Main content: Left (browser 3/4) + Right (steps 1/4)
        // ═══════════════════════════════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // ── Left Panel: Web Browser ─────────────────
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

                    // Step 1 header
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        color: "#f8fafc"

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: "#e8ecf2"
                        }

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
                            border.color: "#e2e8f0"
                            border.width: 1
                            radius: 6

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
                radius: 10
                color: "#ffffff"
                border.color: "#e5e9f0"
                border.width: 1

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
                    anchors.margins: 16
                    spacing: 12

                    Label {
                        text: "后续步骤"
                        color: "#64748b"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    // ── Step 2 ──
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100
                        radius: 8
                        color: "#f8fafc"
                        border.color: "#e2e8f0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Label {
                                text: "步骤2：匹配并移动字幕"
                                color: "#111827"
                                font.pixelSize: 13
                                font.bold: true
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
                        Layout.preferredHeight: 100
                        radius: 8
                        color: "#f8fafc"
                        border.color: "#e2e8f0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Label {
                                text: "步骤3：合成视频+字幕"
                                color: "#111827"
                                font.pixelSize: 13
                                font.bold: true
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
                                spacing: 6

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
                                }

                                IconButton {
                                    Layout.preferredWidth: 72
                                    implicitHeight: 28
                                    text: "执行"
                                    tooltip: "合成视频+字幕"
                                    normalColor: controller && controller.isProcessing ? "#94a3b8" : "#8b5cf6"
                                    hoverColor: "#7c3aed"
                                    borderColor: "#7c3aed"
                                    textColor: "#ffffff"
                                    enabled: !controller || !controller.isProcessing
                                    onClicked: {
                                        if (controller)
                                            controller.mergeSubtitleToVideo();
                                    }
                                }
                            }
                        }
                    }

                    // ── Step 4 ──
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100
                        radius: 8
                        color: "#f8fafc"
                        border.color: "#e2e8f0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Label {
                                text: "步骤4：匹配+替换原视频"
                                color: "#111827"
                                font.pixelSize: 13
                                font.bold: true
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
                                    normalColor: controller && controller.isProcessing ? "#94a3b8" : "#dc2626"
                                    hoverColor: "#b91c1c"
                                    borderColor: "#b91c1c"
                                    textColor: "#ffffff"
                                    enabled: !controller || !controller.isProcessing
                                    onClicked: {
                                        if (controller)
                                            controller.replaceOriginalVideo();
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    // ── Status / Progress ──
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: progressRow.implicitHeight + 12
                        radius: 6
                        color: controller && controller.statusMessage ? "#eff6ff" : "transparent"
                        visible: controller ? (controller.isProcessing || controller.statusMessage.length > 0) : false

                        ColumnLayout {
                            id: progressRow
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 3

                            Label {
                                Layout.fillWidth: true
                                text: controller ? controller.statusMessage : ""
                                color: "#475569"
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                visible: controller ? controller.isProcessing : false

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 3
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
                                    font.pixelSize: 10
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }
        }

        // ═══════════════════════════════════════
        // Bottom: One-line real-time log
        // ═══════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 32
            radius: 6
            color: _lastLogLine.length > 0 ? "#f1f5f9" : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: {
                        if (_lastLogLine.charAt(0) === '✗')
                            return "#dc2626";
                        if (_lastLogLine.charAt(0) === '✓')
                            return "#059669";
                        return "#3b82f6";
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: _lastLogLine
                    color: "#475569"
                    font.pixelSize: 11
                    font.family: "Consolas, 'Courier New', monospace"
                    elide: Text.ElideRight
                }

                IconButton {
                    implicitWidth: 24
                    implicitHeight: 24
                    iconSource: "qrc:/icons/trash.svg"
                    tooltip: "清空日志"
                    visible: _lastLogLine.length > 0
                    normalColor: "transparent"
                    hoverColor: "#e2e8f0"
                    borderColor: "transparent"
                    onClicked: {
                        _logEntries = [];
                        _lastLogLine = "";
                    }
                }
            }
        }
    }
}
