import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

Pane {
    id: root
    property var pal: themeManager.palette

    signal backRequested

    property var controller: null

    padding: 0
    background: Rectangle {
        id: pageBg
        color: pal.CustomSubtitlePage_pageBg_color
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
                implicitHeight: 38
                tooltip: "返回"
                onClicked: root.backRequested()
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    id: titleLabel
                    text: "自定义视频字幕"
                    color: pal.CustomSubtitlePage_titleLabel_color
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: descLabel
                    text: "下载字幕->匹配视频字幕->合成视频+字幕->替换原视频，一站式完成"
                    color: pal.CustomSubtitlePage_descLabel_color
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
            id: settingsPanel
            Layout.fillWidth: true
            Layout.fillHeight: false
            implicitHeight: settingsColumn.implicitHeight + 36
            radius: 10
            color: pal.CustomSubtitlePage_settingsPanel_color
            border.color: pal.CustomSubtitlePage_settingsPanel_borderColor
            border.width: 1

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
                        id: subtitleDownloadLabel
                        text: "字幕下载路径"
                        color: pal.CustomSubtitlePage_subtitleDownloadLabel_color
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.subtitleDownloadPath : ""
                        readOnly: true
                        placeholderText: "字幕文件下载后保存的目录路径"
                        font.pixelSize: 11
                    }

                    IconButton {
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
                        id: videoSourceLabel
                        text: "原视频路径"
                        color: pal.CustomSubtitlePage_videoSourceLabel_color
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.videoSourcePath : ""
                        readOnly: true
                        placeholderText: "存放原视频的目录路径"
                        font.pixelSize: 11
                    }

                    CheckBoxEx {
                        id: recursiveCheck
                        implicitWidth: 110
                        text: "递归子文件夹"
                        textColor: pal.checkBox_textColor
                        checked: controller ? controller.recursive : false
                        font.pixelSize: 11
                        onCheckedChanged: {
                            if (controller)
                                controller.recursive = checked;
                        }
                    }

                    IconButton {
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
                        id: mergedOutputLabel
                        text: "合成输出路径"
                        color: pal.CustomSubtitlePage_mergedOutputLabel_color
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.mergedOutputPath : ""
                        readOnly: true
                        placeholderText: "合成视频+字幕后，文件的输出路径"
                        font.pixelSize: 11
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择合成输出路径"
                        onClicked: mergedOutputFolderDialog.open()
                    }
                }

                // Row 4: FFmpeg 路径
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: ffmpegLabel
                        text: "FFmpeg路径"
                        color: pal.CustomSubtitlePage_ffmpegLabel_color
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.ffmpegPath : ""
                        readOnly: true
                        placeholderText: "选择 ffmpeg.exe 路径（用于合成视频+字幕）"
                        font.pixelSize: 11
                    }

                    Label {
                        id: downloadLink
                        text: "下载地址 →"
                        color: pal.CustomSubtitlePage_downloadLink_color
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

                    IconButton {
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
            id: statusBar
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            radius: 6
            color: (controller && controller.statusMessage) ? pal.CustomSubtitlePage_statusBar_color_active : pal.CustomSubtitlePage_statusBar_color_idle

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 2

                // Row 1: real-time log + current file
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        id: statusMsgLabel
                        Layout.fillWidth: true
                        text: {
                            var msg = _lastLogLine.length > 0 ? _lastLogLine
                                   : (controller && controller.statusMessage.length > 0 ? controller.statusMessage : "");
                            return msg.length > 0 ? msg.replace(/[\r\n]+/g, " ") : "就绪";
                        }
                        color: {
                            var hasMsg = _lastLogLine.length > 0 || (controller && controller.statusMessage.length > 0);
                            return hasMsg ? pal.CustomSubtitlePage_statusMsgLabel_color_log : pal.CustomSubtitlePage_statusMsgLabel_color_idle;
                        }
                        font.pixelSize: 11
                        font.family: "Consolas, 'Courier New', monospace"
                        elide: Text.ElideRight
                    }

                    Label {
                        id: currentFileLabel
                        visible: controller && controller.isProcessing
                                 && controller.currentFile.length > 0
                        text: "[" + controller.currentFile + "]"
                        color: pal.CustomSubtitlePage_currentFileLabel_color
                        font.pixelSize: 11
                        font.family: "Consolas, 'Courier New', monospace"
                        font.bold: true
                    }
                }

                // Row 2: search progress（字幕搜索进度）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: browserCtrl && browserCtrl.searching && browserCtrl.searchProgress > 0

                    Rectangle {
                        id: searchProgressBg
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                        Layout.alignment: Qt.AlignVCenter
                        radius: 2
                        color: pal.CustomSubtitlePage_searchProgressBg_color

                        Rectangle {
                            id: searchProgressFill
                            width: parent.width * (browserCtrl ? browserCtrl.searchProgress / 100 : 0)
                            height: parent.height
                            radius: 2
                            color: pal.CustomSubtitlePage_searchProgressFill_color
                        }
                    }

                    Label {
                        id: searchProgressPct
                        text: browserCtrl ? browserCtrl.searchProgressMessage : ""
                        color: pal.CustomSubtitlePage_searchProgressPct_color
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                // Row 3: progress（步骤3→当前文件的 ffmpeg 实时进度，步骤2/4→走总进度 N/M）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: controller ? controller.isProcessing : false

                    Rectangle {
                        id: ffmpegProgressBg
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                        Layout.alignment: Qt.AlignVCenter
                        radius: 2
                        color: pal.CustomSubtitlePage_ffmpegProgressBg_color
                        visible: controller && controller.currentStep === controller.stepMerge

                        Rectangle {
                            id: ffmpegProgressFill
                            width: parent.width * (controller ? controller.currentFileProgress : 0)
                            height: parent.height
                            radius: 2
                            color: pal.CustomSubtitlePage_ffmpegProgressFill_color
                        }
                    }

                    Item {
                        id: replaceStepKeepSpace
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                        visible: controller && controller.currentStep === controller.stepReplace
                    }

                    Label {
                        id: ffmpegProgressPct
                        text: controller && controller.currentStep === controller.stepMerge
                              ? Math.round(controller.currentFileProgress * 100) + "%"
                              : (controller.totalCount > 0
                                 ? controller.processedCount + "/" + controller.totalCount
                                 : "")
                        color: pal.CustomSubtitlePage_ffmpegProgressPct_color
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
                id: leftPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: pal.CustomSubtitlePage_leftPanel_color
                border.color: pal.CustomSubtitlePage_leftPanel_borderColor
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
                                id: step1Title
                                text: "步骤1：搜索并下载字幕"
                                color: pal.CustomSubtitlePage_step1Title_color
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                id: step1Hint
                                text: "下载的文件将保存到字幕下载路径"
                                color: pal.CustomSubtitlePage_step1Hint_color
                                font.pixelSize: 10
                            }
                        }
                    }

                    // separator
                    Rectangle {
                        id: sep1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: pal.CustomSubtitlePage_sep1_color
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
                            ComboBoxEx {
                                id: siteCombo
                                Layout.preferredWidth: 140
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
                            ComboBoxEx {
                                id: langFilterCombo
                                Layout.preferredWidth: 120
                                font.pixelSize: 12
                                currentIndex: 0

                                model: browserCtrl ? browserCtrl.languageFilterOptions : []
                                textRole: "display"
                                valueRole: "code"

                                onCurrentValueChanged: {
                                    if (browserCtrl)
                                        browserCtrl.languageFilter = currentValue;
                                }

                                Component.onCompleted: {
                                    if (!browserCtrl) return
                                    var opts = browserCtrl.languageFilterOptions;
                                    var cur  = browserCtrl.languageFilter;
                                    for (var i = 0; i < opts.length; i++) {
                                        if (opts[i].code === cur) {
                                            currentIndex = i;
                                            break;
                                        }
                                    }
                                }
                            }

                            // 关键字输入框
                            Rectangle {
                                id: keywordInputBg
                                Layout.fillWidth: true
                                implicitHeight: 32
                                radius: 4
                                color: pal.CustomSubtitlePage_keywordInputBg_color
                                border.color: keywordInput.activeFocus ? pal.CustomSubtitlePage_keywordInputBg_borderColor_active : pal.CustomSubtitlePage_keywordInputBg_borderColor_normal
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
                                    color: pal.CustomSubtitlePage_keywordField_color
                                    placeholderText: "输入关键字搜索字幕..."
                                    selectByMouse: true
                                    onTextChanged: {
                                        if (browserCtrl) browserCtrl.keyword = text;
                                    }
                                    onAccepted: searchBtn.clicked()
                                }
                            }

                            // 搜索/停止按钮
                            IconButton {
                                id: searchBtn
                                Layout.preferredWidth: 90
                                text: browserCtrl && browserCtrl.searching ? "停止搜索" : "搜索"
                                tooltip: browserCtrl && browserCtrl.searching ? "停止搜索" : "搜索字幕"
                                normalColor: browserCtrl && browserCtrl.searching ? pal.CustomSubtitlePage_searchBtn_normalColor_active : pal.CustomSubtitlePage_searchBtn_normalColor_normal
                                hoverColor: browserCtrl && browserCtrl.searching ? pal.CustomSubtitlePage_searchBtn_hoverColor_active : pal.CustomSubtitlePage_searchBtn_hoverColor_normal
                                borderColor: normalColor
                                textColor: pal.CustomSubtitlePage_searchBtn_textColor
                                onClicked: {
                                    if (browserCtrl && browserCtrl.searching) {
                                        browserCtrl.stopSearch();
                                    } else {
                                        if (keywordInput.text.trim().length <= 0)
                                            return;
                                        if (browserCtrl) {
                                            browserCtrl.keyword = keywordInput.text.trim();
                                            browserCtrl.search();
                                        }
                                    }
                                }
                            }

                            BusyIndicator {
                                id: searchBusyIndicator
                                Layout.preferredWidth: 30
                                Layout.preferredHeight: 30
                                Layout.maximumWidth: 30
                                Layout.maximumHeight: 30
                                running: browserCtrl ? browserCtrl.searching : false
                                visible: running
                            }
                            IconButton {
                                id: clearSearchRecords
                                Layout.preferredWidth: 30
                                iconSource: "qrc:/icons/trash.svg"
                                tooltip: "清空记录"
                                visible: !searchBusyIndicator.visible
                                enabled: !searchBusyIndicator.visible && searchResultsModel.count > 0
                                onClicked: searchResultsModel.clear()
                            }
                        }
                    }

                    // separator
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: pal.CustomSubtitlePage_sep1_color
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
                                    id: pyWarnEmoji
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "⚠️"
                                    font.pixelSize: 28
                                    color: pal.CustomSubtitlePage_pyWarnEmoji_color
                                }
                                Label {
                                    id: pyWarnTitle
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "Python 不可用"
                                    color: pal.CustomSubtitlePage_pyWarnTitle_color
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    id: pyWarnDesc
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请安装 Python 3.10+，并确保在系统 PATH 中"
                                    color: pal.CustomSubtitlePage_pyWarnDesc_color
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
                                    id: depWarnEmoji
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "📦"
                                    font.pixelSize: 28
                                    color: pal.CustomSubtitlePage_depWarnEmoji_color
                                }
                                Label {
                                    id: depWarnTitle
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "缺少 Python 依赖库"
                                    color: pal.CustomSubtitlePage_depWarnTitle_color
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    id: depWarnDesc
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请安装 lxml 库"
                                    color: pal.CustomSubtitlePage_depWarnDesc_color
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 320
                                    wrapMode: Text.WordWrap
                                }
                                TextField {
                                    id: pipCmdField
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "pip install lxml"
                                    color: pal.CustomSubtitlePage_pipCmdField_color
                                    font.pixelSize: 12
                                    font.family: "Consolas, 'Courier New', monospace"
                                    font.bold: true
                                    padding: 6
                                    background: Rectangle {
                                        id: pipCmdBg
                                        radius: 4
                                        color: pal.CustomSubtitlePage_pipCmdBg_color
                                        border.color: pal.CustomSubtitlePage_pipCmdBg_borderColor
                                        border.width: 1
                                    }
                                }
                                Button {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "检查安装"
                                    onClicked: browserCtrl.checkDependencies()
                                    background: Rectangle {
                                        id: installBtnBg
                                        radius: 4
                                        color: parent.hovered ? pal.CustomSubtitlePage_installBtnBg_color_hover : pal.CustomSubtitlePage_installBtnBg_color_normal
                                        border.color: pal.CustomSubtitlePage_installBtnBg_borderColor
                                        border.width: 1
                                    }
                                    contentItem: Text {
                                        id: installBtnText
                                        text: parent.text
                                        color: pal.CustomSubtitlePage_installBtnText_color
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
                                    id: emptySearchEmoji
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "🔍"
                                    font.pixelSize: 28
                                    color: pal.CustomSubtitlePage_emptySearchEmoji_color
                                }
                                Label {
                                    id: emptySearchTitle
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "输入关键字搜索字幕"
                                    color: pal.CustomSubtitlePage_emptySearchTitle_color
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    id: emptySearchDesc
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "选择左侧网站，输入片名或关键字，点击搜索"
                                    color: pal.CustomSubtitlePage_emptySearchDesc_color
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
                                id: resultItem
                                width: searchResultsList.width
                                height: 48
                                radius: 4
                                color: itemMouse.containsMouse ? pal.CustomSubtitlePage_resultItem_color_hover : pal.CustomSubtitlePage_resultItem_color_normal
                                border.color: itemMouse.containsMouse ? pal.CustomSubtitlePage_resultItem_borderColor_hover : pal.CustomSubtitlePage_resultItem_borderColor_normal
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 8
                                    spacing: 10

                                    // 语言标签
                                    Rectangle {
                                        id: langTag
                                        Layout.preferredWidth: 52
                                        Layout.preferredHeight: 22
                                        radius: 4
                                        color: pal.CustomSubtitlePage_langTag_color

                                        Label {
                                            id: langLabel
                                            anchors.centerIn: parent
                                            text: model.language || ""
                                            color: pal.CustomSubtitlePage_langLabel_color
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }

                                    // 文件名
                                    Label {
                                        id: fileNameLabel
                                        Layout.fillWidth: true
                                        text: model.fileName || ""
                                        color: pal.CustomSubtitlePage_fileNameLabel_color
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }

                                    // 站点来源
                                    Label {
                                        id: siteLabel
                                        text: model.site || ""
                                        color: pal.CustomSubtitlePage_siteLabel_color
                                        font.pixelSize: 10
                                        Layout.preferredWidth: 80
                                        horizontalAlignment: Text.AlignRight
                                    }

                                    // 下载按钮
                                    IconButton {
                                        id: downloadBtn
                                        Layout.preferredWidth: 64
                                        text: "下载"
                                        tooltip: "下载此字幕文件"
                                        normalColor: pal.CustomSubtitlePage_downloadBtn_normalColor
                                        hoverColor: pal.CustomSubtitlePage_downloadBtn_hoverColor
                                        borderColor: pal.CustomSubtitlePage_downloadBtn_borderColor
                                        textColor: pal.CustomSubtitlePage_downloadBtn_textColor
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
                        id: step2Panel
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: 8
                        color: pal.CustomSubtitlePage_step2Panel_color
                        border.color: pal.CustomSubtitlePage_step2Panel_borderColor
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5

                            RowLayout {
                                spacing: 5

                                Label {
                                    id: step2Title
                                    text: "步骤2：匹配并移动字幕"
                                    color: pal.CustomSubtitlePage_step2Title_color
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.fillWidth: true
                                }

                                BusyIndicator {
                                    width: 30
                                    height: 30
                                    implicitWidth: 30
                                    implicitHeight: 30
                                    running: controller && controller.currentStep === controller.stepMatch
                                    visible: running
                                }
                            }

                            Label {
                                id: step2Desc
                                text: "自动匹配下载的字幕与视频，重命名并移动到视频目录"
                                color: pal.CustomSubtitlePage_step2Desc_color
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                ComboBoxEx {
                                    id: preprocessCombo
                                    Layout.fillWidth: true
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
                                    id: step2ExecBtn
                                    Layout.preferredWidth: 64
                                    text: "执行"
                                    tooltip: "匹配并移动字幕"
                                    normalColor: controller && controller.isProcessing ? pal.CustomSubtitlePage_step2ExecBtn_normalColor_disabled : pal.CustomSubtitlePage_step2ExecBtn_normalColor_normal
                                    hoverColor: pal.CustomSubtitlePage_step2ExecBtn_hoverColor
                                    borderColor: pal.CustomSubtitlePage_step2ExecBtn_borderColor
                                    textColor: pal.CustomSubtitlePage_step2ExecBtn_textColor
                                    visible: !controller || !controller.isProcessing
                                    onClicked: {
                                        if (controller)
                                            controller.matchAndMoveSubtitles();
                                    }
                                }

                                Item {
                                    id: setp2KeepSpace
                                    Layout.fillWidth: true
                                    visible: controller && controller.isProcessing && controller.currentStep === controller.stepMatch
                                }

                                IconButton {
                                    id: step2StopBtn
                                    Layout.preferredWidth: 64
                                    text: "立即停止"
                                    tooltip: "完成当前字幕的匹配、移动和预处理后停止"
                                    normalColor: pal.CustomSubtitlePage_step3StopBtn_normalColor
                                    hoverColor: pal.CustomSubtitlePage_step3StopBtn_hoverColor
                                    borderColor: pal.CustomSubtitlePage_step3StopBtn_borderColor
                                    textColor: pal.CustomSubtitlePage_step3StopBtn_textColor
                                    enabled: controller && controller.isProcessing && controller.currentStep === controller.stepMatch && !controller.stopRequested
                                    visible: enabled
                                    onClicked: {
                                        if (controller)
                                            controller.cancel();
                                    }
                                }
                             }

                        }
                    }

                    // ── Step 3 ──
                    Rectangle {
                        id: step3Panel
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: 8
                        color: pal.CustomSubtitlePage_step3Panel_color
                        border.color: pal.CustomSubtitlePage_step3Panel_borderColor
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5

                            RowLayout {
                                spacing: 5

                                Label {
                                    id: step3Title
                                    text: "步骤3：合成视频+字幕"
                                    color: pal.CustomSubtitlePage_step3Title_color
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.fillWidth: true
                                }

                                BusyIndicator {
                                    width: 30
                                    height: 30
                                    implicitWidth: 30
                                    implicitHeight: 30
                                    running: controller && controller.currentStep === controller.stepMerge
                                    visible: running
                                }
                            }

                            Label {
                                id: step3Desc
                                text: "将字幕嵌入视频，生成带字幕的新视频文件"
                                color: pal.CustomSubtitlePage_step3Desc_color
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                // 空闲状态：执行按钮
                                CheckBoxEx {
                                    Layout.fillWidth: true
                                    text: "GPU加速"
                                    textColor: pal.checkBox_textColor
                                    checked: controller ? controller.gpuAccel : false
                                    font.pixelSize: 11
                                    visible: !controller || !controller.isProcessing
                                    onCheckedChanged: {
                                        if (controller)
                                            controller.gpuAccel = checked;
                                    }
                                }

                                IconButton {
                                    id: step3ExecBtn
                                    Layout.preferredWidth: 72
                                    text: "执行"
                                    tooltip: "合成视频+字幕"
                                    normalColor: pal.CustomSubtitlePage_step3ExecBtn_normalColor
                                    hoverColor: pal.CustomSubtitlePage_step3ExecBtn_hoverColor
                                    borderColor: pal.CustomSubtitlePage_step3ExecBtn_borderColor
                                    textColor: pal.CustomSubtitlePage_step3ExecBtn_textColor
                                    visible: !controller || !controller.isProcessing
                                    onClicked: {
                                        if (controller)
                                            controller.mergeSubtitleToVideo();
                                    }
                                }

                                // 预约停止
                                RowLayout {
                                    spacing: 4
                                    visible: controller && controller.isProcessing && controller.currentStep === controller.stepMerge

                                    Label {
                                        id: step3CompleteLabel
                                        text: "再完成"
                                        color: pal.CustomSubtitlePage_step3CompleteLabel_color
                                        font.pixelSize: 11
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    ComboBoxEx {
                                        id: stopAfterCombo
                                        model: ["全部", 1, 2, 3, 5, 10, 20, 30, 40, 50]
                                        currentIndex: 0
                                        implicitWidth: 70
                                        font.pixelSize: 11
                                        Layout.alignment: Qt.AlignVCenter
                                        onActivated: {
                                            // 选中即生效，无需额外点击按钮
                                            if (controller && controller.isProcessing) {
                                                var val = currentValue;
                                                if (typeof val === "number") {
                                                    controller.requestStopAfterCount(val);
                                                } else {
                                                    // "全部" → 取消预约停止
                                                    controller.requestStopAfterCount(0);
                                                }
                                            }
                                        }
                                    }

                                    Label {
                                        id: step3StopAfterLabel
                                        text: "个后停止"
                                        color: pal.CustomSubtitlePage_step3StopAfterLabel_color
                                        font.pixelSize: 11
                                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                        Layout.fillWidth: true
                                    }
                                }

                                // 立即停止
                                IconButton {
                                    id: step3StopBtn
                                    Layout.preferredWidth: 72
                                    text: "立即停止"
                                    tooltip: "强制终止当前合成任务"
                                    normalColor: pal.CustomSubtitlePage_step3StopBtn_normalColor
                                    hoverColor: pal.CustomSubtitlePage_step3StopBtn_hoverColor
                                    borderColor: pal.CustomSubtitlePage_step3StopBtn_borderColor
                                    textColor: pal.CustomSubtitlePage_step3StopBtn_textColor
                                    enabled: controller && controller.isProcessing && controller.currentStep === controller.stepMerge
                                    visible: enabled
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
                        id: step4Panel
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: 8
                        color: pal.CustomSubtitlePage_step4Panel_color
                        border.color: pal.CustomSubtitlePage_step4Panel_borderColor
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5

                            RowLayout {
                                spacing: 5

                                Label {
                                    id: step4Title
                                    text: "步骤4：匹配+替换原视频"
                                    color: pal.CustomSubtitlePage_step4Title_color
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.fillWidth: true
                                }

                                BusyIndicator {
                                    width: 30
                                    height: 30
                                    implicitWidth: 30
                                    implicitHeight: 30
                                    running: controller && controller.currentStep === controller.stepReplace
                                    visible: running
                                }
                            }

                            Label {
                                id: step4Desc
                                text: "用合成后的视频替换原文件，并清理同名字幕"
                                color: pal.CustomSubtitlePage_step4Desc_color
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                CheckBoxEx {
                                    Layout.fillWidth: true
                                    text: "备份原文件"
                                    textColor: pal.checkBox_textColor
                                    checked: controller ? controller.backupOriginal : false
                                    font.pixelSize: 11
                                    visible: !controller || !controller.isProcessing
                                    onCheckedChanged: {
                                        if (controller)
                                            controller.backupOriginal = checked;
                                    }
                                }

                                IconButton {
                                    id: step4ExecBtn
                                    Layout.preferredWidth: 72
                                    text: "执行"
                                    tooltip: "替换原视频"
                                    normalColor: pal.CustomSubtitlePage_step4ExecBtn_normalColor
                                    hoverColor: pal.CustomSubtitlePage_step4ExecBtn_hoverColor
                                    borderColor: pal.CustomSubtitlePage_step4ExecBtn_borderColor
                                    textColor: pal.CustomSubtitlePage_step4ExecBtn_textColor
                                    visible: !controller || !controller.isProcessing
                                    onClicked: {
                                        if (controller)
                                            controller.replaceOriginalVideo();
                                    }
                                }

                                Item {
                                    id: setp4KeepSpace
                                    Layout.fillWidth: true
                                    visible: controller && controller.isProcessing && controller.currentStep === controller.stepReplace
                                }

                                IconButton {
                                    id: step4StopBtn
                                    Layout.preferredWidth: 72
                                    text: "立即停止"
                                    tooltip: "完成当前视频的备份、替换和清理字幕文件后停止"
                                    normalColor: pal.CustomSubtitlePage_step3StopBtn_normalColor
                                    hoverColor: pal.CustomSubtitlePage_step3StopBtn_hoverColor
                                    borderColor: pal.CustomSubtitlePage_step3StopBtn_borderColor
                                    textColor: pal.CustomSubtitlePage_step3StopBtn_textColor
                                    enabled: controller && controller.isProcessing && controller.currentStep === controller.stepReplace && !controller.stopRequested
                                    visible: enabled
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
