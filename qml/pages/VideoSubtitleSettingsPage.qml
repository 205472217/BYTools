import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

Pane {
    id: root

    signal backRequested

    property var settings: null

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

    FolderDialog {
        id: modelDirDialog
        title: "选择模型下载目录"
        onAccepted: {
            if (settings) {
                settings.whisperModelDir = decodeURIComponent(selectedFolder.toString().replace("file:///", ""));
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
                anchors.fill: parent
                anchors.margins: 18
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                ColumnLayout {
                    width: parent.width
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

                Label {
                    text: settings ? settings.ffmpegStatus : ""
                    color: settings && settings.ffmpegStatus.indexOf("检测到") >= 0 ? "#059669" : "#dc2626"
                    font.pixelSize: 12
                    leftPadding: 92
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

                Label {
                    text: settings ? settings.whisperStatus : ""
                    color: settings && settings.whisperStatus.indexOf("检测到") >= 0 ? "#059669" : "#dc2626"
                    font.pixelSize: 12
                    leftPadding: 92
                }

                // Local model file
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "本地模型"
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

                // Download model
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "下载模型"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    ComboBoxEx {
                        Layout.preferredWidth: 120
                        model: ["tiny", "base", "small", "medium", "large"]
                        currentIndex: settings ? settings.whisperModel : 3
                        onActivated: {
                            if (settings)
                                settings.whisperModel = currentIndex;
                        }
                    }

                    Label {
                        text: settings && settings.availableModels && settings.whisperModel < settings.availableModels.length ? (settings.availableModels[settings.whisperModel].downloaded ? "✅ 已下载" : "未下载") : ""
                        color: "#64748b"
                        font.pixelSize: 12
                    }
                }

                ListView {
                    id: modelListView
                    Layout.fillWidth: true
                    implicitHeight: contentHeight
                    interactive: false
                    spacing: 4
                    model: settings ? settings.availableModels : null

                    delegate: RowLayout {
                        width: modelListView.width
                        spacing: 8

                        Label {
                            Layout.preferredWidth: 56
                            text: modelData.name
                            color: "#334155"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Label {
                            Layout.preferredWidth: 64
                            text: Math.round(modelData.fileSize / 1048576) + " MB"
                            color: "#94a3b8"
                            font.pixelSize: 11
                        }

                        Rectangle {
                            Layout.preferredWidth: 8
                            Layout.preferredHeight: 8
                            radius: 4
                            Layout.alignment: Qt.AlignVCenter
                            color: modelData.downloaded ? "#22c55e" : "#cbd5e1"
                        }

                        Label {
                            visible: index === 3
                            text: "推荐"
                            color: "#2563eb"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            text: modelData.downloaded ? "删除" : "下载"
                            color: modelData.downloaded ? "#dc2626" : "#2563eb"
                            font.pixelSize: 12
                            font.bold: true

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -6
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (modelData.downloaded) {
                                        settings.deleteModel(index);
                                    } else {
                                        Qt.openUrlExternally("https://huggingface.co/ggerganov/whisper.cpp/tree/main");
                                    }
                                }
                            }
                        }
                    }
                }

                Label {
                    text: settings ? "模型目录: " + settings.whisperModelDir : ""
                    color: "#c7d2e0"
                    font.pixelSize: 11
                }

                // Model download directory
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "下载目录"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    Label {
                        Layout.fillWidth: true
                        text: settings ? settings.whisperModelDir : ""
                        color: "#64748b"
                        font.pixelSize: 12
                        elide: Text.ElideMiddle
                        clip: true
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "更改目录"
                        onClicked: modelDirDialog.open()
                    }
                }

                // Translation engine
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "翻译引擎"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    ComboBoxEx {
                        Layout.preferredWidth: 180
                        model: ["百度翻译", "不翻译"]
                        currentIndex: settings ? (settings.translateEngine === 3 ? 1 : 0) : 0
                        onActivated: {
                            if (settings) {
                                // Map: index 0 → engine 0 (百度), index 1 → engine 3 (不翻译)
                                settings.translateEngine = currentIndex === 0 ? 0 : 3;
                            }
                        }
                    }
                }

                // API Key
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true
                    visible: settings ? settings.translateEngine === 0 : false

                    Label {
                        text: "API Key"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        id: apiKeyField
                        Layout.fillWidth: true
                        text: settings ? settings.apiKey : ""
                        echoMode: TextInput.Password
                        placeholderText: "输入 API Key"
                        onTextChanged: {
                            if (settings)
                                settings.apiKey = text;
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/eye.svg"
                        tooltip: apiKeyField.echoMode === TextInput.Password ? "显示" : "隐藏"
                        onClicked: {
                            apiKeyField.echoMode = apiKeyField.echoMode === TextInput.Password ? TextInput.Normal : TextInput.Password;
                        }
                    }
                }

                // Baidu App ID
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true
                    visible: settings ? settings.translateEngine === 0 : false

                    Label {
                        text: "App ID"
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

                // API URL
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true
                    visible: settings ? settings.translateEngine === 0 : false

                    Label {
                        text: "API 地址"
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
                }

                // Test connection
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true
                    visible: settings ? settings.translateEngine === 0 : false

                    Item {
                        Layout.preferredWidth: 80
                    }

                    IconButton {
                        text: "测试连接"
                        enabled: settings ? !settings.apiTesting : false
                        onClicked: {
                            if (settings)
                                settings.testApiConnection();
                        }
                    }

                    Label {
                        text: settings ? settings.apiTestResult : ""
                        color: settings && settings.apiTestResult.indexOf("正常") >= 0 ? "#059669" : "#dc2626"
                        font.pixelSize: 12
                    }
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
                }

            }

        }

        // Action buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            IconButton {
                implicitWidth: 120
                implicitHeight: 40
                text: "恢复默认"
                onClicked: {
                    if (settings)
                        settings.resetDefaults();
                }
            }

            Item {
                Layout.fillWidth: true
            }

            IconButton {
                implicitWidth: 120
                implicitHeight: 40
                text: "保存设置"
                iconSource: "qrc:/icons/play.svg"
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
