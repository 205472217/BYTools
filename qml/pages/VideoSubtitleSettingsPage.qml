import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

Pane {
    id: root

    property var pal: themeManager.palette

    signal backRequested

    property QtObject settings: pluginManager.getPluginSettings("video-subtitle")
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
        id: pageBg
        color: pal.SurfaceEx_pageBg
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
        spacing: 14

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            IconButton {
                iconSource: "qrc:/icons/arrow-left.svg"
                implicitHeight: 38
                tooltip: "返回"
                paletteGroup: "IconBtnEx"
                onClicked: root.backRequested()
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    id: pageTitle
                    text: "插件设置"
                    color: pal.LabelEx_titleText
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: pageSubtitle
                    text: "配置工具路径、翻译 API 和字幕样式"
                    color: pal.LabelEx_subtitleText
                    font.pixelSize: 14
                }
            }

            Item {
                Layout.fillWidth: true
            }

            IconButton {
                id: saveBtn
                implicitWidth: 120
                implicitHeight: 38
                text: "保存设置"
                paletteGroup: "VideoSubtitleSettingsPage_saveBtn"
                onClicked: {
                    if (settings) {
                        settings.saveSettings();
                        root.backRequested();
                    }
                }
            }
        }

        // Settings card
        Rectangle {
            id: settingsCard
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: pal.SurfaceEx_cardBg
            border.color: pal.SurfaceEx_cardBorder
            border.width: 1

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
                            id: ffmpegLabel
                            text: "FFmpeg"
                            color: pal.LabelEx_labelText
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        TextFieldEx {
                            Layout.fillWidth: true
                            text: settings ? settings.ffmpegPath : ""
                            placeholderText: "FFmpeg 可执行文件路径"
                            readOnly: true
                            paletteGroup: "TextFieldEx"
                        }

                        IconButton {
                            iconSource: "qrc:/icons/folder.svg"
                            tooltip: "浏览"
                            paletteGroup: "IconBtnEx"
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
                            id: ffmpegStatusLabel
                            text: {
                                if (!settings) return "";
                                if (settings.ffmpegDetecting) return "检测中...";
                                if (settings.ffmpegPath && settings.ffmpegPath.length > 0) {
                                    var parts = settings.ffmpegPath.replace(/\\/g, '/').split('/');
                                    var ok = settings.ffmpegStatus.indexOf("已找到") >= 0 || settings.ffmpegStatus.indexOf("已检测到") >= 0;
                                    return "已选择 FFmpeg: " + parts[parts.length - 1] + (ok ? " — 可用" : " — 不可用");
                                }
                                return "请点击下载地址进行 FFmpeg 下载";
                            }
                            color: {
                                if (!settings) return "";
                                if (settings.ffmpegDetecting) return pal.LabelEx_warningText;
                                if (settings.ffmpegPath && settings.ffmpegPath.length > 0) {
                                    var ok = settings.ffmpegStatus.indexOf("已找到") >= 0 || settings.ffmpegStatus.indexOf("已检测到") >= 0;
                                    return ok ? pal.LabelEx_successText : pal.LabelEx_errorText;
                                }
                                return pal.LabelEx_errorText;
                            }
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            id: ffmpegDownloadLabel
                            text: "下载地址 →"
                            color: pal.LabelEx_statusText
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
                            id: whisperLabel
                            text: "Whisper"
                            color: pal.LabelEx_labelText
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        TextFieldEx {
                            Layout.fillWidth: true
                            text: settings ? settings.whisperPath : ""
                            placeholderText: "whisper-cli.exe 路径"
                            readOnly: true
                            paletteGroup: "TextFieldEx"
                        }

                        IconButton {
                            iconSource: "qrc:/icons/folder.svg"
                            tooltip: "浏览"
                            paletteGroup: "IconBtnEx"
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
                            id: whisperStatusLabel
                            text: {
                                if (!settings) return "";
                                if (settings.whisperDetecting) return "检测中...";
                                if (settings.whisperPath && settings.whisperPath.length > 0) {
                                    var parts = settings.whisperPath.replace(/\\/g, '/').split('/');
                                    var ok = settings.whisperStatus.indexOf("已找到") >= 0 || settings.whisperStatus.indexOf("已检测到") >= 0;
                                    return "已选择 Whisper: " + parts[parts.length - 1] + (ok ? " — 可用" : " — 不可用");
                                }
                                return "请点击下载地址进行 Whisper 下载";
                            }
                            color: {
                                if (!settings) return "";
                                if (settings.whisperDetecting) return pal.LabelEx_warningText;
                                if (settings.whisperPath && settings.whisperPath.length > 0) {
                                    var ok = settings.whisperStatus.indexOf("已找到") >= 0 || settings.whisperStatus.indexOf("已检测到") >= 0;
                                    return ok ? pal.LabelEx_successText : pal.LabelEx_errorText;
                                }
                                return pal.LabelEx_errorText;
                            }
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            id: whisperDownloadLabel
                            text: "下载地址 →"
                            color: pal.LabelEx_statusText
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
                            id: modelLabel
                            text: "Whisper模型"
                            color: pal.LabelEx_labelText
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        TextFieldEx {
                            Layout.fillWidth: true
                            text: settings ? settings.localModelPath : ""
                            placeholderText: "选择本地 .bin / .ggml 模型文件"
                            readOnly: true
                            paletteGroup: "TextFieldEx"
                        }

                        IconButton {
                            iconSource: "qrc:/icons/folder.svg"
                            tooltip: "浏览"
                            paletteGroup: "IconBtnEx"
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
                            color: settings && settings.localModelPath && settings.localModelPath.length > 0 ? pal.LabelEx_successText : pal.LabelEx_errorText
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        // Download hyperlink (right-aligned with browse button above)
                        Label {
                            id: modelDownloadLabel
                            text: "下载地址 →"
                            color: pal.LabelEx_statusText
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
                        id: sectionDivider1
                        Layout.fillWidth: true
                        height: 1
                        color: pal.SurfaceEx_divider
                    }

                    // Engine selector — 选择翻译引擎（后端: translateEngine 属性）
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true
                        Layout.bottomMargin: 4

                        Label {
                            id: engineLabel
                            text: "翻译引擎"
                            color: pal.LabelEx_labelText
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
                            paletteGroup: "ComboBoxEx"
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
                                id: baiduAppIdLabel
                                text: "百度 App ID"
                                color: pal.LabelEx_labelText
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
                                paletteGroup: "TextFieldEx"
                            }
                        }

                        // Baidu Secret Key (后端: apiKey 属性)
                        RowLayout {
                            spacing: 12
                            Layout.fillWidth: true

                            Label {
                                id: baiduApiKeyLabel
                                text: "百度 API Key"
                                color: pal.LabelEx_labelText
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
                                paletteGroup: "TextFieldEx"
                            }

                            IconButton {
                                iconSource: "qrc:/icons/eye.svg"
                                tooltip: baiduApiKeyField.echoMode === TextInput.Password ? "显示" : "隐藏"
                                paletteGroup: "IconBtnEx"
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
                                id: baiduApiUrlLabel
                                text: "百度 API 地址"
                                color: pal.LabelEx_labelText
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
                                paletteGroup: "TextFieldEx"
                            }

                            IconButton {
                                id: testConnectionBtn
                                text: "测试连接"
                                paletteGroup: "VideoSubtitleSettingsPage_testConnectionBtn"
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
                                id: apiTestResultLabel
                                text: settings ? settings.apiTestResult : ""
                                color: settings && settings.apiTestResult.indexOf("正常") >= 0 ? pal.LabelEx_successText : pal.LabelEx_errorText
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
                                id: libreUrlLabel
                                text: "服务地址"
                                color: pal.LabelEx_labelText
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
                                paletteGroup: "TextFieldEx"
                            }

                            IconButton {
                                id: libreTestBtn
                                text: "测试连接"
                                paletteGroup: "VideoSubtitleSettingsPage_libreTestBtn"
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
                                id: libreStatusLabel
                                text: settings ? settings.libreTranslateStatus : ""
                                color: settings && settings.libreTranslateStatus.indexOf("正常") >= 0 ? pal.LabelEx_successText : pal.LabelEx_errorText
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
                                id: libreConfigTitle
                                text: "LibreTranslate 配置说明（离线翻译）"
                                color: pal.LabelEx_subtitleText
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Label {
                                id: libreDescription
                                text: "LibreTranslate 是一款开源的离线神经机器翻译引擎，翻译质量高，无需联网。"
                                color: pal.LabelEx_infoText
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
                                    id: bulletLabel
                                    text: "•"
                                    color: pal.LabelEx_infoText
                                    font.pixelSize: 12
                                }

                                Label {
                                    id: pythonReqLabel
                                    text: "需要 Python 3.8+ 环境"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 12
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    id: pythonDownloadLabel
                                    text: "Python 下载 →"
                                    color: pal.LabelEx_statusText
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
                                    id: step1Label
                                    text: "步骤 1: 打开终端（cmd），执行："
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    spacing: 0
                                    Layout.fillWidth: true

                                    Rectangle {
                                        id: pipCmdBg
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 32
                                        radius: 4
                                        color: pal.VideoSubtitleSettingsPage_pipCmdBg_color

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
                                            color: pal.LabelEx_codeText
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
                                        color: root.copyFeedback === "pip" ? pal.LabelEx_successText : pal.LabelEx_infoText
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
                                    id: step2Label
                                    text: "步骤 2: 启动服务（仅加载需要的语言: en=英语,zh=中文,ja=日语,ko=韩语）"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    spacing: 0
                                    Layout.fillWidth: true

                                    Rectangle {
                                        id: ltCmdBg
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 32
                                        radius: 4
                                        color: pal.VideoSubtitleSettingsPage_ltCmdBg_color

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
                                            color: pal.LabelEx_codeText
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
                                        color: root.copyFeedback === "lt" ? pal.LabelEx_successText : pal.LabelEx_infoText
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
                                    id: libreDescriptionText
                                    text: "首次启动会自动下载翻译模型（约 500MB-2GB），下载完成后即可离线使用。<br>出现Running on http://127.0.0.1:5000即为成功，不要关闭窗口。<br>服务默认监听 <b>http://localhost:5000</b>，可在上方修改地址。"
                                    color: pal.LabelEx_infoText
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    textFormat: Text.RichText
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    id: libreGitHubLabel
                                    text: "LibreTranslate GitHub →"
                                    color: pal.LabelEx_statusText
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
                        id: sectionDivider2
                        Layout.fillWidth: true
                        height: 1
                        color: pal.SurfaceEx_divider
                    }

                    // Font size + border width
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            id: fontSizeLabel
                            text: "字号"
                            color: pal.LabelEx_labelText
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
                            paletteGroup: "ComboBoxEx"
                        }

                        Rectangle {
                            id: fontSizeDivider
                            width: 1
                            height: 24
                            color: pal.SurfaceEx_divider
                        }

                        Label {
                            id: borderWidthLabel
                            text: "描边宽度"
                            color: pal.LabelEx_labelText
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
                            paletteGroup: "ComboBoxEx"
                        }
                    }

                    // Font color + border color
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            id: subtitleStyleLabel
                            text: "字幕样式"
                            color: pal.LabelEx_labelText
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        ComboBoxEx {
                            Layout.preferredWidth: 100
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
                            paletteGroup: "ComboBoxEx"
                        }

                        Rectangle {
                            id: subtitleStyleDivider
                            width: 1
                            height: 24
                            color: pal.SurfaceEx_divider
                        }

                        Label {
                            id: fontColorLabel
                            text: "字体颜色"
                            color: pal.LabelEx_labelText
                            font.pixelSize: 13
                            font.bold: true
                        }

                        Rectangle {
                            id: fontColorSwatch
                            width: 28
                            height: 28
                            radius: 6
                            color: settings ? settings.defaultFontColor : pal.VideoSubtitleSettingsPage_fontColorSwatch_color
                            border.width: 2
                            border.color: pal.VideoSubtitleSettingsPage_fontColorSwatch_borderColor

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: fontColorDialog.open()
                            }
                        }

                        Label {
                            id: fontColorValue
                            color: pal.LabelEx_valueText
                            font.pixelSize: 12
                        }

                        Rectangle {
                            id: styleDivider
                            width: 1
                            height: 24
                            color: pal.SurfaceEx_divider
                        }

                        Label {
                            id: borderColorLabel
                            text: "描边颜色"
                            color: pal.LabelEx_labelText
                            font.pixelSize: 13
                            font.bold: true
                        }

                        Rectangle {
                            id: borderColorSwatch
                            width: 28
                            height: 28
                            radius: 6
                            color: settings ? settings.defaultBorderColor : pal.VideoSubtitleSettingsPage_borderColorSwatch_color
                            border.width: 2
                            border.color: pal.VideoSubtitleSettingsPage_borderColorSwatch_borderColor

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: borderColorDialog.open()
                            }
                        }

                        Label {
                            id: borderColorValue
                            color: pal.LabelEx_valueText
                            font.pixelSize: 12
                        }
                    }

                    // Preview
                    Rectangle {
                        id: previewBg
                        Layout.fillWidth: true
                        height: 56
                        radius: 6
                        color: pal.VideoSubtitleSettingsPage_previewBg_color
                        border.width: 1
                        border.color: pal.SurfaceEx_cardBorder

                        Label {
                            id: previewLabel
                            anchors.centerIn: parent
                            text: "这是一行字幕文字预览"
                            color: settings ? settings.defaultFontColor : pal.VideoSubtitleSettingsPage_previewLabel_color
                            font.pixelSize: settings ? settings.defaultFontSize : 20
                            style: Text.Outline
                            styleColor: settings ? settings.defaultBorderColor : pal.VideoSubtitleSettingsPage_previewLabel_styleColor
                        }
                    }

                    // 分割线
                    Rectangle {
                        id: sectionDivider3
                        Layout.fillWidth: true
                        height: 1
                        color: pal.SurfaceEx_divider
                    }

                    // GPU 加速
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            id: gpuLabel
                            text: "GPU 加速"
                            color: pal.LabelEx_labelText
                            font.pixelSize: 13
                            font.bold: true
                            Layout.preferredWidth: 80
                        }

                        CheckBoxEx {
                            id: gpuCheckBox
                            implicitWidth: 250
                            text: "启用硬件加速（NVENC / QSV / AMF）"
                            paletteGroup: "CheckBoxEx"
                            checked: settings ? settings.useGpuAccel : false
                            enabled: settings ? (settings.ffmpegPath.length > 0 && (settings.ffmpegDetecting || settings.ffmpegStatus.indexOf("已找到") >= 0 || settings.ffmpegStatus.indexOf("已检测到") >= 0)) : false
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
                            id: gpuInfoLabel
                            text: settings ? settings.gpuAccelInfo : ""
                            color: {
                                if (!settings) return pal.LabelEx_infoText;
                                if (settings.ffmpegDetecting) return pal.LabelEx_warningText;
                                if (settings.gpuAccelInfo.indexOf("NVENC") >= 0
                                    || settings.gpuAccelInfo.indexOf("QSV") >= 0
                                    || settings.gpuAccelInfo.indexOf("AMF") >= 0)
                                    return pal.LabelEx_successText;
                                return pal.LabelEx_infoText;
                            }
                            font.pixelSize: 12
                        }
                    }

                    // Output file retention
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            id: retentionLabel
                            text: "保留输出文件"
                            color: pal.LabelEx_labelText
                            font.pixelSize: 13
                            font.bold: true
                            topPadding: 8
                            bottomPadding: 4
                        }

                        CheckBoxEx {
                            id: keepWavCheckBox
                            implicitWidth: 150
                            text: "保留 WAV 音频文件"
                            paletteGroup: "CheckBoxEx"
                            checked: settings ? settings.keepWav : true
                            onCheckedChanged: {
                                if (settings)
                                    settings.keepWav = checked;
                            }
                        }

                        CheckBoxEx {
                            id: keepOrigSrtCheckBox
                            implicitWidth: 150
                            text: "保留原始 SRT 字幕"
                            paletteGroup: "CheckBoxEx"
                            checked: settings ? settings.keepOriginalSrt : true
                            onCheckedChanged: {
                                if (settings)
                                    settings.keepOriginalSrt = checked;
                            }
                        }

                        CheckBoxEx {
                            id: keepTranslatedSrtCheckBox
                            implicitWidth: 150
                            text: "保留翻译后 SRT"
                            paletteGroup: "CheckBoxEx"
                            checked: settings ? settings.keepTranslatedSrt : true
                            onCheckedChanged: {
                                if (settings)
                                    settings.keepTranslatedSrt = checked;
                            }
                        }

                        Item { Layout.fillWidth: true }

                        IconButton {
                            id: keepToggleBtn
                            property bool allKept: true
                            Binding on allKept {
                                value: settings ? (settings.keepWav && settings.keepOriginalSrt && settings.keepTranslatedSrt) : true
                            }
                            text: allKept ? "全部取消" : "全部保留"
                            tooltip: allKept ? "取消勾选所有保留选项" : "勾选所有保留选项"
                            normalColor: allKept ? pal.VideoSubtitleSettingsPage_keepToggleBtn_normalColor_active : pal.VideoSubtitleSettingsPage_keepToggleBtn_normalColor_normal
                            hoverColor: allKept ? pal.VideoSubtitleSettingsPage_keepToggleBtn_hoverColor_active : pal.VideoSubtitleSettingsPage_keepToggleBtn_hoverColor_normal
                            borderColor: pal.VideoSubtitleSettingsPage_keepToggleBtn_borderColor
                            textColor: allKept ? pal.VideoSubtitleSettingsPage_keepToggleBtn_textColor_active : pal.VideoSubtitleSettingsPage_keepToggleBtn_textColor_normal
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
    }
}
