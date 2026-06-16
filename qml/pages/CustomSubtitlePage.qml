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

    // ── Browser controller shortcut ──
    property var browserCtrl: controller ? controller.browserController : null

    // ── 字幕预处理同步 ──
    function _syncPreprocessors() {
        if (!controller) return;
        var list = [];
        for (var i = 0; i < _preprocessModel.count; ++i) {
            if (_preprocessModel.get(i).checked)
                list.push(_preprocessModel.get(i).key);
        }
        controller.enabledPreprocessors = list;
    }

    function _loadPreprocessors() {
        if (!controller) return;
        var stored = controller.enabledPreprocessors;
        for (var i = 0; i < _preprocessModel.count; ++i) {
            _preprocessModel.get(i).checked = stored.indexOf(_preprocessModel.get(i).key) >= 0;
        }
    }

    ListModel {
        id: _preprocessModel
        ListElement { key: "removeEnvSound"; label: "去除环境音"; tooltip: "移除字幕中被括号包裹的环境音字幕，如 (music)、[Applause]、（掌声）"; checked: false }
        ListElement { key: "removeBgSound";  label: "去除背景音"; tooltip: "移除字幕中被星号包裹的音效字幕，如 *叮咚*、*门铃声*"; checked: false }
        ListElement { key: "removeMusic";    label: "过滤歌词";   tooltip: "移除字幕中被音符标记 ♪ 包裹的音乐歌词字幕，如 ♪ Happy Birthday ♪"; checked: false }
        ListElement { key: "dedup";          label: "去重";       tooltip: "移除字幕中相邻重复的字幕文本，连续相同内容只保留第一条"; checked: false }
        ListElement { key: "t2s";            label: "中文繁转简"; tooltip: "将字幕中的繁体中文转换为简体中文"; checked: false }
    }

    Connections {
        target: controller
        function onLogMessage(message) {
            if (message.length === 0)
                return;
            _lastLogLine = message;
        }
    }

    // ── 浏览器控制器数据同步 ──
    Connections {
        target: browserCtrl
        function onSearchResultsChanged() {
            searchResultsModel.clear();
            if (!browserCtrl) return;
            var results = browserCtrl.searchResults;
            for (var i = 0; i < results.length; ++i) {
                searchResultsModel.append(results[i]);
            }
        }
        function onSearchingChanged() {
            searchBusyIndicator.running = browserCtrl ? browserCtrl.searching : false;
        }
        function onDownloadingChanged() {
            downloadBusyIndicator.running = browserCtrl ? browserCtrl.downloading : false;
        }
        function onCurrentSiteChanged() {
            if (!browserCtrl) return;
            var idx = siteCombo.find(browserCtrl.currentSite);
            if (idx >= 0) siteCombo.currentIndex = idx;
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

                // Row 2: progress（步骤3→当前文件的 ffmpeg 实时进度，步骤2/4→走总进度 N/M）
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
                            width: parent.width * (controller ? controller.currentFileProgress : 0)
                            height: parent.height
                            radius: 2
                            color: "#2563eb"
                        }
                    }

                    Label {
                        text: controller && controller.currentStep === "合成视频+字幕"
                              ? Math.round(controller.currentFileProgress * 100) + "%"
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
        // Main content: Left (字幕搜索 3/4) + Right (自定义步骤 1/4)
        // ═══════════════════════════════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // ── Left Panel: 字幕搜索 ─────────────────
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
                                text: "步骤1：搜索并下载字幕"
                                color: "#111827"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

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

                    // ── Search bar (fixed height) ──
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            anchors.topMargin: 8
                            anchors.bottomMargin: 8
                            spacing: 8

                            // 网站下拉框
                            ComboBox {
                                id: siteCombo
                                Layout.preferredWidth: 140
                                implicitHeight: 32
                                font.pixelSize: 12
                                currentIndex: 0

                                model: browserCtrl ? browserCtrl.availableSites : []

                                onCurrentIndexChanged: {
                                    if (browserCtrl && currentIndex >= 0)
                                        browserCtrl.currentSite = currentText;
                                }

                                Component.onCompleted: {
                                    if (browserCtrl) {
                                        var idx = find(browserCtrl.currentSite);
                                        if (idx >= 0) currentIndex = idx;
                                    }
                                }
                            }

                            // 语言筛选下拉框
                            ComboBox {
                                id: langFilterCombo
                                Layout.preferredWidth: 100
                                implicitHeight: 32
                                font.pixelSize: 12
                                currentIndex: 0

                                model: ["全部语言", "中文简体", "中文繁体", "英文"]

                                onCurrentTextChanged: {
                                    if (browserCtrl)
                                        browserCtrl.languageFilter = currentText;
                                }

                                Component.onCompleted: {
                                    if (browserCtrl)
                                        currentIndex = find(browserCtrl.languageFilter);
                                }
                            }

                            // 关键字输入框
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 32
                                radius: 4
                                color: "#ffffff"
                                border.color: keywordInput.activeFocus ? "#2563eb" : "#e2e8f0"
                                border.width: 1

                                TextField {
                                    id: keywordInput
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    topPadding: 4
                                    bottomPadding: 4
                                    background: Item {}
                                    verticalAlignment: TextInput.AlignVCenter
                                    font.pixelSize: 12
                                    color: "#334155"
                                    placeholderText: "输入关键字搜索字幕..."
                                    selectByMouse: true
                                    onTextChanged: {
                                        if (browserCtrl) browserCtrl.keyword = text;
                                    }
                                    onAccepted: searchBtn.clicked()
                                }
                            }

                            // 搜索按钮
                            IconButton {
                                id: searchBtn
                                Layout.preferredWidth: 80
                                implicitHeight: 32
                                text: "搜索"
                                tooltip: "搜索字幕"
                                normalColor: "#2563eb"
                                hoverColor: "#1d4ed8"
                                borderColor: "#1d4ed8"
                                textColor: "#ffffff"
                                enabled: !(browserCtrl && browserCtrl.searching)
                                onClicked: {
                                    if (keywordInput.text.trim().length <= 0)
                                        return;
                                    if (browserCtrl) {
                                        browserCtrl.keyword = keywordInput.text.trim();
                                        browserCtrl.search();
                                    }
                                }
                            }

                            BusyIndicator {
                                id: searchBusyIndicator
                                width: 28
                                height: 28
                                running: browserCtrl ? browserCtrl.searching : false
                                visible: running
                            }
                            Item {
                                id: searchBusyIndicatorKeepSize
                                width: 28
                                height: 28
                                visible: !searchBusyIndicator.visible
                            }
                        }
                    }

                    // separator
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#e2e8f0"
                    }

                    // ── Search results list ──
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "transparent"

                        // 空状态提示
                        Column {
                            anchors.centerIn: parent
                            spacing: 10
                            visible: searchResultsModel.count === 0 && !searchBusyIndicator.running

                            // Python 不可用警告
                            Column {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: 8
                                visible: browserCtrl && !browserCtrl.pythonAvailable

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "⚠️"
                                    font.pixelSize: 28
                                    color: "#f59e0b"
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "Python 不可用"
                                    color: "#92400e"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请安装 Python 3.10+，并确保在系统 PATH 中"
                                    color: "#b45309"
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 320
                                    wrapMode: Text.WordWrap
                                }
                            }

                            // Python 依赖缺失警告
                            Column {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: 8
                                visible: browserCtrl && browserCtrl.pythonAvailable && !browserCtrl.dependenciesMet

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "📦"
                                    font.pixelSize: 28
                                    color: "#f59e0b"
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "缺少 Python 依赖库"
                                    color: "#92400e"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请安装 lxml 库"
                                    color: "#b45309"
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 320
                                    wrapMode: Text.WordWrap
                                }
                                TextField {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "pip install lxml"
                                    color: "#92400e"
                                    font.pixelSize: 12
                                    font.family: "Consolas, 'Courier New', monospace"
                                    font.bold: true
                                    padding: 6
                                    background: Rectangle { radius: 4; color: "#fef3c7"; border.color: "#fde68a"; border.width: 1 }
                                }
                                Button {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "检查安装"
                                    onClicked: browserCtrl.checkDependencies()
                                    background: Rectangle {
                                        radius: 4
                                        color: parent.hovered ? "#d97706" : "#f59e0b"
                                        border.color: "#d97706"
                                        border.width: 1
                                    }
                                    contentItem: Text {
                                        text: parent.text
                                        color: "#ffffff"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }

                            // 正常空状态
                            Column {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: 10
                                visible: !browserCtrl || (browserCtrl.pythonAvailable && browserCtrl.dependenciesMet)

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "🔍"
                                    font.pixelSize: 28
                                    color: "#cbd5e1"
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "输入关键字搜索字幕"
                                    color: "#94a3b8"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "选择左侧网站，输入片名或关键字，点击搜索"
                                    color: "#c7d2e0"
                                    font.pixelSize: 12
                                }
                            }
                        }

                        // 结果列表
                        ListView {
                            id: searchResultsList
                            anchors.fill: parent
                            anchors.margins: 4
                            clip: true
                            spacing: 2
                            visible: searchResultsModel.count > 0

                            model: ListModel {
                                id: searchResultsModel
                                // 从 controller.browserController.searchResults 同步
                                // ListElement {
                                //     site: "SubtitleCat"
                                //     language: "English"
                                //     fileName: "The.Matrix.1999.srt"
                                //     downloadUrl: "https://..."
                                // }
                            }

                            delegate: Rectangle {
                                width: searchResultsList.width
                                height: 48
                                radius: 4
                                color: itemMouse.containsMouse ? "#eff6ff" : "#ffffff"
                                border.color: itemMouse.containsMouse ? "#bfdbfe" : "#f1f5f9"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 8
                                    spacing: 10

                                    // 语言标签
                                    Rectangle {
                                        Layout.preferredWidth: 52
                                        Layout.preferredHeight: 22
                                        radius: 4
                                        color: "#dbeafe"

                                        Label {
                                            anchors.centerIn: parent
                                            text: model.language || ""
                                            color: "#2563eb"
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }

                                    // 文件名
                                    Label {
                                        Layout.fillWidth: true
                                        text: model.fileName || ""
                                        color: "#334155"
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }

                                    // 站点来源
                                    Label {
                                        text: model.site || ""
                                        color: "#94a3b8"
                                        font.pixelSize: 10
                                        Layout.preferredWidth: 80
                                        horizontalAlignment: Text.AlignRight
                                    }

                                    // 下载按钮
                                    IconButton {
                                        Layout.preferredWidth: 64
                                        implicitHeight: 28
                                        text: "下载"
                                        tooltip: "下载此字幕文件"
                                        normalColor: "#16a34a"
                                        hoverColor: "#15803d"
                                        borderColor: "#15803d"
                                        textColor: "#ffffff"
                                        enabled: browserCtrl && !browserCtrl.downloading
                                        onClicked: {
                                            if (browserCtrl) browserCtrl.download(index);
                                        }
                                    }
                                }

                                MouseArea {
                                    id: itemMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    // 不拦截下载按钮的点击
                                    propagateComposedEvents: true
                                    onPressed: mouse.accepted = false
                                }
                            }
                        }

                        BusyIndicator {
                            id: downloadBusyIndicator
                            anchors.centerIn: parent
                            width: 40
                            height: 40
                            running: browserCtrl ? browserCtrl.downloading : false
                            visible: running
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
                                    width: 30
                                    height: 30
                                    implicitWidth: 30
                                    implicitHeight: 30
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

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                ComboBox {
                                    id: preprocessCombo
                                    Layout.fillWidth: true
                                    implicitHeight: 28
                                    font.pixelSize: 11
                                    model: _preprocessModel
                                    visible: !controller || !controller.isProcessing

                                    delegate: ItemDelegate {
                                        width: parent.width
                                        contentItem: RowLayout {
                                            spacing: 8
                                            Label {
                                                text: model.checked ? "☑" : "☐"
                                                font.pixelSize: 14
                                            }
                                            Label {
                                                text: model.label
                                                font.pixelSize: 11
                                                Layout.fillWidth: true
                                            }
                                        }
                                        ToolTip {
                                            text: model.tooltip
                                            visible: parent.hovered
                                            delay: 600
                                            font.pixelSize: 11
                                        }
                                        onClicked: {
                                            model.checked = !model.checked;
                                            _syncPreprocessors();
                                        }
                                    }

                                    displayText: "字幕处理"

                                    Component.onCompleted: {
                                        _loadPreprocessors();
                                    }
                                }

                                IconButton {
                                    Layout.preferredWidth: 64
                                    implicitHeight: 28
                                    text: "执行"
                                    tooltip: "匹配并移动字幕"
                                    normalColor: controller && controller.isProcessing ? "#94a3b8" : "#3b82f6"
                                    hoverColor: "#2563eb"
                                    borderColor: "#2563eb"
                                    textColor: "#ffffff"
                                    visible: !controller || !controller.isProcessing
                                    onClicked: {
                                        if (controller)
                                            controller.matchAndMoveSubtitles();
                                    }
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
                                    width: 30
                                    height: 30
                                    implicitWidth: 30
                                    implicitHeight: 30
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
                                    visible: !controller || !controller.isProcessing
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
                                    normalColor: "#8b5cf6"
                                    hoverColor: "#7c3aed"
                                    borderColor: "#7c3aed"
                                    textColor: "#ffffff"
                                    visible: !controller || !controller.isProcessing
                                    onClicked: {
                                        if (controller)
                                            controller.mergeSubtitleToVideo();
                                    }
                                }

                                // 运行状态：停止控制
                                RowLayout {
                                    spacing: 4
                                    visible: controller ? controller.isProcessing : false

                                    Label {
                                        text: "完成"
                                        color: "#475569"
                                        font.pixelSize: 11
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    ComboBox {
                                        id: stopAfterCombo
                                        model: ["全部", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
                                        currentIndex: 0
                                        implicitWidth: 60
                                        implicitHeight: 26
                                        font.pixelSize: 11
                                        Layout.alignment: Qt.AlignVCenter
                                        onActivated: {
                                            // 选中即生效，无需额外点击按钮
                                            if (controller && controller.isProcessing) {
                                                var val = currentValue;
                                                if (typeof val === "number")
                                                    controller.requestStopAfterCount(val);
                                            }
                                        }
                                    }

                                    Label {
                                        text: "个后停止"
                                        color: "#475569"
                                        font.pixelSize: 11
                                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                        Layout.fillWidth: true
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
                                    width: 30
                                    height: 30
                                    implicitWidth: 30
                                    implicitHeight: 30
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
                                    visible: !controller || !controller.isProcessing
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
                                    visible: !controller || !controller.isProcessing
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
                }
            }
        }
    }
}
