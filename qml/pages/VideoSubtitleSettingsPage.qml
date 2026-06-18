import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

Pane {
    id: root

    signal backRequested

    property var settings: null
    property string copyFeedback: ""

    Timer {
        id: copyTimer
        interval: 3000
        onTriggered: root.copyFeedback = ""
    }

    Component.onCompleted: {
        if (settings && typeof settings.loadSettings === 'function') {
            settings.loadSettings();
        }
    }

    padding: 0
    background: Rectangle {
        color: "#f4f6f9"
    }

    FileDialog {
        id: ffmpegFileDialog
        title: "选择 FFmpeg 可执行文件"
        nameFilters: ["可执行文件 (*.exe)", "所有文件 (*)"]
        onAccepted: {
            if (settings) {
                settings.ffmpegPath = decodeURIComponent(selectedFile.toString().replace("file:///", ""));
            }
        }
    }

    FileDialog {
        id: whisperFileDialog
        title: "选择 whisper-cli.exe"
        nameFilters: ["可执行文件 (*.exe)", "所有文件 (*)"]
        onAccepted: {
            if (settings) {
                settings.whisperPath = decodeURIComponent(selectedFile.toString().replace("file:///", ""));
            }
        }
    }

    FileDialog {
        id: modelFileDialog
        title: "选择 Whisper 模型文件"
        nameFilters: ["模型文件 (*.bin *.ggml)", "所有文件 (*)"]
        onAccepted: {
            if (settings) {
                settings.localModelPath = decodeURIComponent(selectedFile.toString().replace("file:///", ""));
            }
        }
    }

    ColorDialog {
        id: fontColorDialog
        selectedColor: settings ? settings.defaultFontColor : "#ffffff"
        onAccepted: {
            if (settings)
                settings.defaultFontColor = selectedColor.toString();
        }
    }

    ColorDialog {
        id: borderColorDialog
        selectedColor: settings ? settings.defaultBorderColor : "#000000"
        onAccepted: {
            if (settings)
                settings.defaultBorderColor = selectedColor.toString();
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
                    text: "插件设置"
                    color: "#111827"
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: "配置工具路径、翻译 API 和字幕样式"
                    color: "#64748b"
                    font.pixelSize: 14
                }
            }
        }

        // Settings card
        Rectangle {
            Layout.fillWidth: true
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

            ScrollView {
                id: contentScrollView
                anchors.fill: parent
                anchors.margins: 18
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AlwaysOn

                ColumnLayout {
                    width: contentScrollView.availableWidth-15
                    spacing: 12

                    // FFmpeg path
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "FFmpeg"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        TextFieldEx {
                            Layout.fillWidth: true
                            text: settings ? settings.ffmpegPath : ""
                            placeholderText: "FFmpeg 可执行文件路径"
                            readOnly: true
                        }

                        IconButton {
                            iconSource: "qrc:/icons/folder.svg"
                            tooltip: "浏览"
                            onClicked: ffmpegFileDialog.open()
                        }
                    }

                    // FFmpeg status + download link
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Item {
                            Layout.preferredWidth: 80
                        }

                        Label {
                            text: {
                                if (!settings) return "";
                                if (settings.ffmpegPath && settings.ffmpegPath.length > 0) {
                                    var parts = settings.ffmpegPath.replace(/\\/g, '/').split('/');
                                    var ok = settings.ffmpegStatus.indexOf("已找到") >= 0 || settings.ffmpegStatus.indexOf("已检测到") >= 0;
                                    return "已选择 FFmpeg: " + parts[parts.length - 1] + (ok ? " — 可用" : " — 不可用");
                                }
                                return "请点击下载地址进行 FFmpeg 下载";
                            }
                            color: settings && settings.ffmpegPath && settings.ffmpegPath.length > 0 && (settings.ffmpegStatus.indexOf("已找到") >= 0 || settings.ffmpegStatus.indexOf("已检测到") >= 0) ? "#059669" : "#dc2626"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "下载地址 →"
                            color: "#2563eb"
                            font.pixelSize: 12
                            font.underline: true

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    Qt.openUrlExternally("https://ffmpeg.org/download.html");
                                }
                            }
                        }
                    }

                    // Whisper path
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "Whisper"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        TextFieldEx {
                            Layout.fillWidth: true
                            text: settings ? settings.whisperPath : ""
                            placeholderText: "whisper-cli.exe 路径"
                            readOnly: true
                        }

                        IconButton {
                            iconSource: "qrc:/icons/folder.svg"
                            tooltip: "浏览"
                            onClicked: whisperFileDialog.open()
                        }
                    }

                    // Whisper status + download link
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Item {
                            Layout.preferredWidth: 80
                        }

                        Label {
                            text: {
                                if (!settings) return "";
                                if (settings.whisperPath && settings.whisperPath.length > 0) {
                                    var parts = settings.whisperPath.replace(/\\/g, '/').split('/');
                                    var ok = settings.whisperStatus.indexOf("已找到") >= 0 || settings.whisperStatus.indexOf("已检测到") >= 0;
                                    return "已选择 Whisper: " + parts[parts.length - 1] + (ok ? " — 可用" : " — 不可用");
                                }
                                return "请点击下载地址进行 Whisper 下载";
                            }
                            color: settings && settings.whisperPath && settings.whisperPath.length > 0 && (settings.whisperStatus.indexOf("已找到") >= 0 || settings.whisperStatus.indexOf("已检测到") >= 0) ? "#059669" : "#dc2626"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "下载地址 →"
                            color: "#2563eb"
                            font.pixelSize: 12
                            font.underline: true

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    Qt.openUrlExternally("https://github.com/ggerganov/whisper.cpp/releases");
                                }
                            }
                        }
                    }

                    // Local model file
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "Whisper模型"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        TextFieldEx {
                            Layout.fillWidth: true
                            text: settings ? settings.localModelPath : ""
                            placeholderText: "选择本地 .bin / .ggml 模型文件"
                            readOnly: true
                        }

                        IconButton {
                            iconSource: "qrc:/icons/folder.svg"
                            tooltip: "浏览"
                            onClicked: modelFileDialog.open()
                        }
                    }

                    // Model status hint & download link (always visible)
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Item {
                            Layout.preferredWidth: 80
                        }

                        Label {
                            id: modelStatusLabel
                            text: {
                                if (!settings) return "";
                                var path = settings.localModelPath;
                                if (path && path.length > 0) {
                                    var parts = path.replace(/\\/g, '/').split('/');
                                    return "已选择模型: " + parts[parts.length - 1];
                                }
                                return "请点击下载地址进行模型下载";
                            }
                            color: settings && settings.localModelPath && settings.localModelPath.length > 0 ? "#059669" : "#dc2626"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        // Download hyperlink (right-aligned with browse button above)
                        Label {
                            text: "下载地址 →"
                            color: "#2563eb"
                            font.pixelSize: 12
                            font.underline: true

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    Qt.openUrlExternally("https://huggingface.co/ggerganov/whisper.cpp/tree/main");
                                }
                            }
                        }
                    }

                    // 分割线
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#e2e8f0"
                    }

                    // Engine selector — 选择翻译引擎（后端: translateEngine 属性）
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true
                        Layout.bottomMargin: 4

                        Label {
                            text: "翻译引擎"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        ComboBoxEx {
                            Layout.preferredWidth: 200
                            model: settings ? settings.translateEngineNames : ["百度翻译"]
                            currentIndex: settings ? settings.translateEngine : 0
                            onActivated: {
                                if (settings)
                                    settings.translateEngine = currentIndex;
                            }
                        }
                    }

                    // ---------- 百度翻译配置 (engine === 0) ----------
                    ColumnLayout {
                        visible: settings ? settings.translateEngine === 0 : true
                        Layout.fillWidth: true
                        spacing: 12

                        // Baidu App ID (后端: baiduAppId 属性)
                        RowLayout {
                            spacing: 12
                            Layout.fillWidth: true

                            Label {
                                text: "百度 App ID"
                                color: "#475569"
                                font.pixelSize: 13
                                font.bold: true
                                Layout.preferredWidth: 80
                            }

                            TextFieldEx {
                                Layout.fillWidth: true
                                text: settings ? settings.baiduAppId : ""
                                placeholderText: "百度翻译 API 的 App ID"
                                onTextChanged: {
                                    if (settings)
                                        settings.baiduAppId = text;
                                }
                            }
                        }

                        // Baidu Secret Key (后端: apiKey 属性)
                        RowLayout {
                            spacing: 12
                            Layout.fillWidth: true

                            Label {
                                text: "百度 API Key"
                                color: "#475569"
                                font.pixelSize: 13
                                font.bold: true
                                Layout.preferredWidth: 80
                            }

                            TextFieldEx {
                                id: baiduApiKeyField
                                Layout.fillWidth: true
                                text: settings ? settings.apiKey : ""
                                echoMode: TextInput.Password
                                placeholderText: "输入百度翻译 Secret Key"
                                onTextChanged: {
                                    if (settings)
                                        settings.apiKey = text;
                                }
                            }

                            IconButton {
                                iconSource: "qrc:/icons/eye.svg"
                                tooltip: baiduApiKeyField.echoMode === TextInput.Password ? "显示" : "隐藏"
                                onClicked: {
                                    baiduApiKeyField.echoMode = baiduApiKeyField.echoMode === TextInput.Password ? TextInput.Normal : TextInput.Password;
                                }
                            }
                        }

                        // Baidu API URL (后端: apiUrl 属性)
                        RowLayout {
                            spacing: 12
                            Layout.fillWidth: true

                            Label {
                                text: "百度 API 地址"
                                color: "#475569"
                                font.pixelSize: 13
                                font.bold: true
                                Layout.preferredWidth: 80
                            }

                            TextFieldEx {
                                Layout.fillWidth: true
                                text: settings ? settings.apiUrl : ""
                                placeholderText: "API 地址"
                                onTextChanged: {
                                    if (settings)
                                        settings.apiUrl = text;
                                }
                            }

                            IconButton {
                                text: "测试连接"
                                normalColor: "#2563eb"
                                hoverColor: "#1d4ed8"
                                borderColor: "#1d4ed8"
                                enabled: settings ? !settings.apiTesting : false
                                onClicked: {
                                    if (settings)
                                        settings.testApiConnection();
                                }
                            }
                        }

                        // Test result
                        RowLayout {
                            spacing: 12
                            Layout.fillWidth: true

                            Item {
                                Layout.preferredWidth: 80
                            }

                            Label {
                                text: settings ? settings.apiTestResult : ""
                                color: settings && settings.apiTestResult.indexOf("正常") >= 0 ? "#059669" : "#dc2626"
                                font.pixelSize: 12
                            }
                        }
                    }
                    // ---------- LibreTranslate 配置 (engine === 1) ----------
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: settings ? settings.translateEngine === 1 : false

                        // LibreTranslate Service URL
                        RowLayout {
                            spacing: 12
                            Layout.fillWidth: true

                            Label {
                                text: "服务地址"
                                color: "#475569"
                                font.pixelSize: 13
                                font.bold: true
                                Layout.preferredWidth: 80
                            }

                            TextFieldEx {
                                Layout.fillWidth: true
                                text: settings ? settings.libreTranslateUrl : ""
                                placeholderText: "http://localhost:5000"
                                onTextChanged: {
                                    if (settings)
                                        settings.libreTranslateUrl = text;
                                }
                            }

                            IconButton {
                                text: "测试连接"
                                normalColor: "#2563eb"
                                hoverColor: "#1d4ed8"
                                borderColor: "#1d4ed8"
                                enabled: settings ? !settings.apiTesting : false
                                onClicked: {
                                    if (settings)
                                        settings.testApiConnection();
                                }
                            }
                        }

                        // LibreTranslate status
                        RowLayout {
                            spacing: 12
                            Layout.fillWidth: true

                            Item {
                                Layout.preferredWidth: 80
                            }

                            Label {
                                text: settings ? settings.libreTranslateStatus : ""
                                color: settings && settings.libreTranslateStatus.indexOf("正常") >= 0 ? "#059669" : "#dc2626"
                                font.pixelSize: 12
                            }
                        }

                        // --- LibreTranslate 安装引导 ---
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Layout.topMargin: 4
                            Layout.bottomMargin: 4

                            Label {
                                text: "LibreTranslate 配置说明（离线翻译）"
                                color: "#1e293b"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Label {
                                text: "LibreTranslate 是一款开源的离线神经机器翻译引擎，翻译质量高，无需联网。"
                                color: "#64748b"
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Item { Layout.preferredHeight: 4 }

                            // 环境要求
                            RowLayout {
                                spacing: 8
                                Layout.fillWidth: true

                                Label {
                                    text: "•"
                                    color: "#94a3b8"
                                    font.pixelSize: 12
                                }

                                Label {
                                    text: "需要 Python 3.8+ 环境"
                                    color: "#334155"
                                    font.pixelSize: 12
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    text: "Python 下载 →"
                                    color: "#2563eb"
                                    font.pixelSize: 12
                                    font.underline: true

                                    MouseArea {
                                        anchors.fill: parent
                                        anchors.margins: -4
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            Qt.openUrlExternally("https://www.python.org/downloads/");
                                        }
                                    }
                                }
                            }

                            // 安装步骤
                            ColumnLayout {
                                spacing: 4
                                Layout.fillWidth: true
                                Layout.leftMargin: 16

                                Label {
                                    text: "步骤 1: 打开终端（cmd），执行："
                                    color: "#475569"
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    spacing: 0
                                    Layout.fillWidth: true

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 32
                                        radius: 4
                                        color: "#f1f5f9"

                                        TextInput {
                                            id: pipCmdText
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.leftMargin: 10
                                            anchors.rightMargin: 10
                                            text: "python -m pip install libretranslate"
                                            font.family: "Consolas, 'Courier New', monospace"
                                            font.pixelSize: 12
                                            color: "#1e293b"
                                            readOnly: true
                                            selectByMouse: true
                                            cursorVisible: false
                                            clip: true
                                        }
                                    }

                                    Item { Layout.preferredWidth: 8 }

                                    Label {
                                        id: pipCopyLabel
                                        text: root.copyFeedback === "pip" ? "已复制" : "复制"
                                        color: root.copyFeedback === "pip" ? "#059669" : "#2563eb"
                                        font.pixelSize: 12
                                        font.bold: true
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.preferredHeight: 32

                                        MouseArea {
                                            anchors.fill: parent
                                            anchors.margins: -4
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                pipCmdText.selectAll();
                                                pipCmdText.copy();
                                                root.copyFeedback = "pip";
                                                copyTimer.start();
                                            }
                                        }
                                    }
                                }

                                Item { Layout.preferredHeight: 2 }

                                Label {
                                    text: "步骤 2: 启动服务（仅加载需要的语言: en=英语,zh=中文,ja=日语,ko=韩语）"
                                    color: "#475569"
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    spacing: 0
                                    Layout.fillWidth: true

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 32
                                        radius: 4
                                        color: "#f1f5f9"

                                        TextInput {
                                            id: ltCmdText
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.leftMargin: 10
                                            anchors.rightMargin: 10
                                            text: "py -m libretranslate.main --load-only en,zh,ja,ko"
                                            font.family: "Consolas, 'Courier New', monospace"
                                            font.pixelSize: 12
                                            color: "#1e293b"
                                            readOnly: true
                                            selectByMouse: true
                                            cursorVisible: false
                                            clip: true
                                        }
                                    }

                                    Item { Layout.preferredWidth: 8 }

                                    Label {
                                        id: ltCopyLabel
                                        text: root.copyFeedback === "lt" ? "已复制" : "复制"
                                        color: root.copyFeedback === "lt" ? "#059669" : "#2563eb"
                                        font.pixelSize: 12
                                        font.bold: true
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.preferredHeight: 32

                                        MouseArea {
                                            anchors.fill: parent
                                            anchors.margins: -4
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                ltCmdText.selectAll();
                                                ltCmdText.copy();
                                                root.copyFeedback = "lt";
                                                copyTimer.start();
                                            }
                                        }
                                    }
                                }
                            }

                            // 说明文字
                            RowLayout {
                                spacing: 8
                                Layout.fillWidth: true
                                Layout.topMargin: 4

                                Label {
                                    text: "首次启动会自动下载翻译模型（约 500MB-2GB），下载完成后即可离线使用。<br>出现Running on http://127.0.0.1:5000即为成功，不要关闭窗口。<br>服务默认监听 <b>http://localhost:5000</b>，可在上方修改地址。"
                                    color: "#64748b"
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    textFormat: Text.RichText
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    text: "LibreTranslate GitHub →"
                                    color: "#2563eb"
                                    font.pixelSize: 12
                                    font.underline: true

                                    MouseArea {
                                        anchors.fill: parent
                                        anchors.margins: -4
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            Qt.openUrlExternally("https://github.com/LibreTranslate/LibreTranslate");
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // 分割线
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#e2e8f0"
                    }

                    // Font size + border width
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "字号"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        ComboBoxEx {
                            Layout.preferredWidth: 100
                            model: ["14px", "16px", "18px", "20px", "24px", "28px", "32px"]
                            currentIndex: settings ? (settings.defaultFontSize - 14) / 2 : 3
                            onActivated: {
                                if (settings)
                                    settings.defaultFontSize = 14 + currentIndex * 2;
                            }
                        }

                        Rectangle {
                            width: 1
                            height: 24
                            color: "#e2e8f0"
                        }

                        Label {
                            text: "描边宽度"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                        }

                        ComboBoxEx {
                            Layout.preferredWidth: 72
                            model: ["0px", "1px", "2px", "3px", "4px"]
                            currentIndex: settings ? settings.defaultBorderWidth : 2
                            onActivated: {
                                if (settings)
                                    settings.defaultBorderWidth = currentIndex;
                            }
                        }
                    }

                    // Font color + border color
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
                            model: ["白色", "蓝色", "红色"]
                            currentIndex: settings ? settings.subtitleStyle : 0
                            onActivated: {
                                if (!settings) return;
                                settings.subtitleStyle = currentIndex;
                                if (currentIndex === 0) {
                                    // 白色样式：白字黑边
                                    settings.defaultFontColor = "#FFFFFF";
                                    settings.defaultBorderColor = "#000000";
                                } else if (currentIndex === 1) {
                                    // 蓝色样式：蓝字白边
                                    settings.defaultFontColor = "#2196F3";
                                    settings.defaultBorderColor = "#FFFFFF";
                                } else if (currentIndex === 2) {
                                    // 红色样式：红字白边
                                    settings.defaultFontColor = "#F44336";
                                    settings.defaultBorderColor = "#FFFFFF";
                                }
                            }
                        }

                        Label {
                            text: "字体颜色"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        Rectangle {
                            width: 28
                            height: 28
                            radius: 6
                            color: settings ? settings.defaultFontColor : "#ffffff"
                            border.width: 2
                            border.color: "#e2e8f0"

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: fontColorDialog.open()
                            }
                        }

                        Label {
                            text: settings ? settings.defaultFontColor : "#FFFFFF"
                            color: "#64748b"
                            font.pixelSize: 12
                        }

                        Rectangle {
                            width: 1
                            height: 24
                            color: "#e2e8f0"
                        }

                        Label {
                            text: "描边颜色"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                        }

                        Rectangle {
                            width: 28
                            height: 28
                            radius: 6
                            color: settings ? settings.defaultBorderColor : "#000000"
                            border.width: 2
                            border.color: "#e2e8f0"

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: borderColorDialog.open()
                            }
                        }

                        Label {
                            text: settings ? settings.defaultBorderColor : "#000000"
                            color: "#64748b"
                            font.pixelSize: 12
                        }
                    }

                    // Preview
                    Rectangle {
                        Layout.fillWidth: true
                        height: 56
                        radius: 6
                        color: "#1a000000"

                        Label {
                            anchors.centerIn: parent
                            text: "这是一行字幕文字预览"
                            color: settings ? settings.defaultFontColor : "#ffffff"
                            font.pixelSize: settings ? settings.defaultFontSize : 20
                            style: Text.Outline
                            styleColor: settings ? settings.defaultBorderColor : "#000000"
                        }
                    }

                    // 分割线
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#e2e8f0"
                    }

                    // GPU 加速
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "GPU 加速"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        CheckBox {
                            text: "启用硬件加速（NVENC / QSV / AMF）"
                            checked: settings ? settings.useGpuAccel : false
                            enabled: settings ? (settings.ffmpegPath.length > 0 && (settings.ffmpegStatus.indexOf("已找到") >= 0 || settings.ffmpegStatus.indexOf("已检测到") >= 0)) : false
                            onCheckedChanged: {
                                if (settings)
                                    settings.useGpuAccel = checked;
                            }
                            ToolTip {
                                text: "使用 GPU 编解码加速字幕烧录，大幅降低 CPU 占用，需要 FFmpeg 支持对应编码器"
                                visible: parent.hovered
                                delay: 400
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: settings ? settings.gpuAccelInfo : ""
                            color: {
                                if (!settings) return "#94a3b8";
                                if (settings.gpuAccelInfo.indexOf("NVENC") >= 0
                                    || settings.gpuAccelInfo.indexOf("QSV") >= 0
                                    || settings.gpuAccelInfo.indexOf("AMF") >= 0)
                                    return "#059669";
                                return "#94a3b8";
                            }
                            font.pixelSize: 12
                        }
                    }

                    // Output file retention
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "保留输出文件"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                            topPadding: 8
                            bottomPadding: 4
                        }

                        CheckBox {
                            text: "保留 WAV 音频文件"
                            checked: settings ? settings.keepWav : true
                            onCheckedChanged: {
                                if (settings)
                                    settings.keepWav = checked;
                            }
                        }

                        CheckBox {
                            text: "保留原始 SRT 字幕"
                            checked: settings ? settings.keepOriginalSrt : true
                            onCheckedChanged: {
                                if (settings)
                                    settings.keepOriginalSrt = checked;
                            }
                        }

                        CheckBox {
                            text: "保留翻译后 SRT"
                            checked: settings ? settings.keepTranslatedSrt : true
                            onCheckedChanged: {
                                if (settings)
                                    settings.keepTranslatedSrt = checked;
                            }
                        }

                        Item { Layout.fillWidth: true }

                        IconButton {
                            property bool allKept: true
                            Binding on allKept {
                                value: settings ? (settings.keepWav && settings.keepOriginalSrt && settings.keepTranslatedSrt) : true
                            }
                            text: allKept ? "全部取消" : "全部保留"
                            tooltip: allKept ? "取消勾选所有保留选项" : "勾选所有保留选项"
                            normalColor: allKept ? "#fef2f2" : "#f0fdf4"
                            hoverColor: allKept ? "#fee2e2" : "#dcfce7"
                            borderColor: "#e2e8f0"
                            textColor: allKept ? "#dc2626" : "#059669"
                            implicitWidth: 100
                            implicitHeight: 32
                            onClicked: {
                                if (settings) {
                                    var newVal = !allKept;
                                    settings.keepWav = newVal;
                                    settings.keepOriginalSrt = newVal;
                                    settings.keepTranslatedSrt = newVal;
                                }
                            }
                        }
                    }
                }

            }

        }

        // Action buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Item {
                Layout.fillWidth: true
            }

            IconButton {
                implicitWidth: 120
                implicitHeight: 40
                text: "保存设置"
                normalColor: "#2563eb"
                hoverColor: "#1d4ed8"
                borderColor: "#1d4ed8"
                onClicked: {
                    if (settings) {
                        settings.saveSettings();
                        root.backRequested();
                    }
                }
            }
        }
    }
}
