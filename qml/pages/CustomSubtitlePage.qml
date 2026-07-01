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
    property var stackView: null
    property string pluginId: ""
    property QtObject settings: pluginManager.settingsForController(controller)

    padding: 0
    background: Rectangle {
        id: pageBg
        color: pal.SurfaceEx_pageBg
    }

    // ── Dialogs ──
    FolderDialog {
        id: subtitleDownloadFolderDialog
        title: "选择字幕下载路径"
        onAccepted: {
            if (controller)
                settings.subtitleDownloadPath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""));
        }
    }

    FolderDialog {
        id: videoSourceFolderDialog
        title: "选择原视频路径"
        onAccepted: {
            if (controller)
                settings.videoSourcePath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""));
        }
    }

    FolderDialog {
        id: mergedOutputFolderDialog
        title: "选择合成视频路径"
        onAccepted: {
            if (controller)
                settings.mergedOutputPath = decodeURIComponent(selectedFolder.toString().replace("file:///", ""));
        }
    }

    FileDialog {
        id: ffmpegFileDialog
        title: "选择 ffmpeg.exe"
        nameFilters: ["ffmpeg (ffmpeg.exe)", "All Files (*)"]
        onAccepted: {
            if (controller)
                settings.ffmpegPath = decodeURIComponent(selectedFile.toString().replace("file:///", ""));
        }
    }

    // ── 返回确认对话框（处理中时提示）─────────────────────────────
    Dialog {
        id: backConfirmDialog
        title: "确认返回"
        modal: true
        anchors.centerIn: parent
        width: 420
        standardButtons: Dialog.NoButton
        closePolicy: Dialog.CloseOnEscape

        contentItem: ColumnLayout {
            spacing: 8
            Layout.margins: 4

            Label {
                text: "当前有任务正在处理中，返回首页将中断执行，是否继续？"
                color: pal.LabelEx_statusText
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
                    id: backCancelBtn
                    text: "取消"
                    tooltip: "不返回，继续当前处理"
                    paletteGroup: "IconBtnEx"
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: backConfirmDialog.close()
                }

                IconButton {
                    id: backConfirmBtn
                    text: "返回首页"
                    tooltip: "中断任务并返回首页"
                    paletteGroup: "BackConfirmBtn"
                    implicitWidth: 120
                    implicitHeight: 38
                    onClicked: {
                        if (controller) { controller.reset() }
                        backConfirmDialog.close()
                        root.backRequested()
                    }
                }
            }
        }
    }

    // ── Log state ──
    property string _lastLogLine: ""

    // ── Unified progress value (0-1) for all steps ──
    property double _progressValue: {
        if (!controller || !controller.isProcessing) return 0;
        if (controller.currentStep === controller.stepSearch)
            return browserCtrl ? browserCtrl.searchProgress / 100 : 0;
        if (controller.currentStep === controller.stepMerge)
            return controller.currentFileProgress;
        return controller.progress;
    }

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
        settings.enabledPreprocessors = list;
    }

    function _loadPreprocessors() {
        if (!controller) return;
        var stored = settings.enabledPreprocessors;
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
        function onCurrentStepChanged() {
            if (controller && controller.currentStep !== controller.stepNone)
                _lastLogLine = "";
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
                paletteGroup: "IconBtnEx"
                onClicked: {
                    if (controller && controller.isProcessing) {
                        backConfirmDialog.open()
                    } else {
                        root.backRequested()
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    id: titleLabel
                    text: "自定义视频字幕"
                    color: pal.LabelEx_titleText
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: descLabel
                    text: "下载字幕->匹配视频字幕->合成视频+字幕->替换原视频，一站式完成"
                    color: pal.LabelEx_subtitleText
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
            color: pal.SurfaceEx_cardBg
            border.color: pal.SurfaceEx_cardBorderLight
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
                        color: pal.LabelEx_labelText
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? settings.subtitleDownloadPath : ""
                        readOnly: true
                        placeholderText: "字幕文件下载后保存的目录路径"
                        font.pixelSize: 11
                        paletteGroup: "TextFieldEx"
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择下载路径"
                        enabled: !controller || controller.currentStep === controller.stepNone
                        paletteGroup: "IconBtnEx"
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
                        color: pal.LabelEx_labelText
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? settings.videoSourcePath : ""
                        readOnly: true
                        placeholderText: "存放原视频的目录路径"
                        font.pixelSize: 11
                        paletteGroup: "TextFieldEx"
                    }

                    CheckBoxEx {
                        id: recursiveCheck
                        implicitWidth: 110
                        text: "递归子文件夹"
                        paletteGroup: "CheckBoxEx"
                        checked: controller ? settings.recursive : false
                        font.pixelSize: 11
                        enabled: !controller || controller.currentStep === controller.stepNone
                        onCheckedChanged: {
                            if (controller)
                                settings.recursive = checked;
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择视频目录"
                        enabled: !controller || controller.currentStep === controller.stepNone
                        paletteGroup: "IconBtnEx"
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
                        color: pal.LabelEx_labelText
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? settings.mergedOutputPath : ""
                        readOnly: true
                        placeholderText: "合成视频+字幕后，文件的输出路径"
                        font.pixelSize: 11
                        paletteGroup: "TextFieldEx"
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择合成输出路径"
                        enabled: !controller || controller.currentStep === controller.stepNone
                        paletteGroup: "IconBtnEx"
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
                        color: pal.LabelEx_labelText
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? settings.ffmpegPath : ""
                        readOnly: true
                        placeholderText: "选择 ffmpeg.exe 路径（用于合成视频+字幕）"
                        font.pixelSize: 11
                        paletteGroup: "TextFieldEx"
                    }

                    Label {
                        id: downloadLink
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

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择 ffmpeg.exe"
                        enabled: !controller || controller.currentStep === controller.stepNone
                        paletteGroup: "IconBtnEx"
                        onClicked: ffmpegFileDialog.open()
                    }
                }
            }
        }

        // ═══════════════════════════════════════
        // Unified status + progress bar (2 rows)
        // ═══════════════════════════════════════
        Rectangle {
            id: statusBar
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            radius: 6
            color: pal.SurfaceEx_statusBar

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 2

                // Row 1: real-time message + current file
                // Shown during all steps and after completion
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: true

                    Label {
                        id: statusMsgLabel
                        Layout.fillWidth: true
                        text: {
                            if (!controller) return "就绪";
                            var isBusy = controller.isProcessing
                                || (browserCtrl && browserCtrl.downloading);
                            if (isBusy)
                                return _lastLogLine.length > 0
                                    ? _lastLogLine.replace(/[\r\n]+/g, " ")
                                    : "";
                            var msg = controller.statusMessage.length > 0
                                ? controller.statusMessage
                                : _lastLogLine;
                            return msg.length > 0
                                ? msg.replace(/[\r\n]+/g, " ")
                                : "就绪";
                        }
                        color: pal.LabelEx_statusText
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }

                    Label {
                        id: currentFileLabel
                        visible: controller && controller.isProcessing
                                 && controller.currentStep >= controller.stepMatch
                                 && controller.currentFile.length > 0
                        text: "[" + controller.currentFile + "]"
                        color: pal.LabelEx_statusText
                        font.pixelSize: 11
                        font.family: "Consolas, 'Courier New', monospace"
                        font.bold: true
                    }
                }

                // Row 2: progress bar + label
                // Shown for all 4 steps during processing
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: controller ? controller.isProcessing : false

                    Rectangle {
                        id: progressBarBg
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                        Layout.alignment: Qt.AlignVCenter
                        radius: 2
                        color: pal.SurfaceEx_progressTrack

                        Rectangle {
                            id: progressBarFill
                            width: parent.width * _progressValue
                            height: parent.height
                            radius: 2
                            color: pal.SurfaceEx_progressFill
                        }
                    }

                    Label {
                        id: progressLabel
                        text: {
                            if (!controller) return "";
                            if (controller.currentStep === controller.stepSearch)
                                return browserCtrl ? browserCtrl.searchProgressMessage : "";
                            if (controller.currentStep === controller.stepMerge)
                                return Math.round(controller.currentFileProgress * 100) + "%";
                            return controller.totalCount > 0
                                ? controller.processedCount + "/" + controller.totalCount
                                : "";
                        }
                        color: pal.LabelEx_statusText
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
                color: pal.SurfaceEx_cardBgAlt
                border.color: pal.SurfaceEx_cardBorderLight
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
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 5

                            Label {
                                id: step1Title
                                text: "步骤1：搜索并下载字幕"
                                color: pal.LabelEx_titleText
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                id: step1Hint
                                text: "下载的文件将保存到字幕下载路径"
                                color: pal.LabelEx_infoText
                                font.pixelSize: 10
                            }
                        }
                    }

                    // separator
                    Rectangle {
                        id: sep1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: pal.SurfaceEx_divider
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
                                enabled: !controller || controller.currentStep === controller.stepNone

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
                                paletteGroup: "ComboBoxEx"
                            }

                            // 语言筛选下拉框
                            ComboBoxEx {
                                id: langFilterCombo
                                Layout.preferredWidth: 120
                                font.pixelSize: 12
                                currentIndex: 0
                                enabled: !controller || controller.currentStep === controller.stepNone

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
                                paletteGroup: "ComboBoxEx"
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
                                    color: pal.TextFieldEx_textColor
                                    placeholderText: "输入关键字搜索字幕..."
                                    selectByMouse: true
                                    enabled: !controller || controller.currentStep === controller.stepNone
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
                                enabled: !controller || (controller.currentStep === controller.stepNone || controller.currentStep === controller.stepSearch)
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
                                enabled: !searchBusyIndicator.visible && searchResultsModel.count > 0 && (!controller || controller.currentStep === controller.stepNone)
                    paletteGroup: "BackCancelBtn"
                                onClicked: searchResultsModel.clear()
                            }
                        }
                    }

                    // separator
                    Rectangle {
                        id: sep2
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: pal.SurfaceEx_divider
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
                                    color: pal.LabelEx_warningText
                                }
                                Label {
                                    id: pyWarnTitle
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "Python 不可用"
                                    color: pal.LabelEx_warningText
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    id: pyWarnDesc
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请安装 Python 3.10+，并确保在系统 PATH 中"
                                    color: pal.LabelEx_warningText
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
                                    color: pal.LabelEx_warningText
                                }
                                Label {
                                    id: depWarnTitle
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "缺少 Python 依赖库"
                                    color: pal.LabelEx_warningText
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    id: depWarnDesc
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请安装 lxml 库"
                                    color: pal.LabelEx_warningText
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 320
                                    wrapMode: Text.WordWrap
                                }
                                TextField {
                                    id: pipCmdField
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "pip install lxml"
                                    color: pal.LabelEx_codeText
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
                                    enabled: !controller || controller.currentStep === controller.stepNone
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
                                    color: pal.LabelEx_infoText
                                }
                                Label {
                                    id: emptySearchTitle
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "输入关键字搜索字幕"
                                    color: pal.LabelEx_infoText
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                Label {
                                    id: emptySearchDesc
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "选择左侧网站，输入片名或关键字，点击搜索"
                                    color: pal.LabelEx_infoText
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
                                            color: pal.LabelEx_statusText
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }

                                    // 文件名
                                    Label {
                                        id: fileNameLabel
                                        Layout.fillWidth: true
                                        text: model.fileName || ""
                                        color: pal.LabelEx_valueText
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }

                                    // 站点来源
                                    Label {
                                        id: siteLabel
                                        text: model.site || ""
                                        color: pal.LabelEx_infoText
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
                                        paletteGroup: "CustomSubtitlePage_downloadBtn"
                                        enabled: browserCtrl && !browserCtrl.downloading && (!controller || controller.currentStep === controller.stepNone)
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
                Layout.preferredWidth: 320
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
                        Layout.preferredHeight: 100
                        radius: 8
                        color: pal.SurfaceEx_cardBgAlt
                        border.color: pal.SurfaceEx_cardBorderLight
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors { topMargin: 2; leftMargin: 10; rightMargin: 10; bottomMargin: 10 }
                            spacing: 5

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                color: "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 0
                                    anchors.rightMargin: 0
                                    spacing: 5

                                    Label {
                                        id: step2Title
                                        text: "步骤2：匹配并移动字幕"
                                        color: pal.LabelEx_titleText
                                        font.pixelSize: 14
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
                            }

                            Label {
                                id: step2Desc
                                text: "自动匹配下载的字幕与视频，重命名并移动到视频目录"
                                color: pal.LabelEx_subtitleText
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 0
                                spacing: 5

                                    ComboBoxEx {
                                        id: preprocessCombo
                                        Layout.fillWidth: true
                                        font.pixelSize: 11
                                        model: _preprocessModel
                                        enabled: !controller || controller.currentStep === controller.stepNone
 
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
                                        paletteGroup: "ComboBoxEx"
                                    }

                                IconButton {
                                    id: step2Btn
                                    Layout.preferredWidth: 64
                                    text: controller && controller.currentStep === controller.stepMatch ? "停止" : "执行"
                                    tooltip: controller && controller.currentStep === controller.stepMatch ? "完成当前字幕的匹配、移动和预处理后停止" : "匹配并移动字幕"
                                    normalColor: controller && controller.currentStep === controller.stepMatch ? pal.CustomSubtitlePage_step3StopBtn_normalColor : pal.CustomSubtitlePage_step2ExecBtn_normalColor_normal
                                    hoverColor: controller && controller.currentStep === controller.stepMatch ? pal.CustomSubtitlePage_step3StopBtn_hoverColor : pal.CustomSubtitlePage_step2ExecBtn_hoverColor
                                    borderColor: controller && controller.currentStep === controller.stepMatch ? pal.CustomSubtitlePage_step3StopBtn_borderColor : pal.CustomSubtitlePage_step2ExecBtn_borderColor
                                    textColor: controller && controller.currentStep === controller.stepMatch ? pal.CustomSubtitlePage_step3StopBtn_textColor : pal.CustomSubtitlePage_step2ExecBtn_textColor
                                    enabled: !controller || controller.currentStep === controller.stepNone || (controller.currentStep === controller.stepMatch && !controller.stopRequested)
                                    onClicked: {
                                        if (!controller) return;
                                        if (controller.currentStep === controller.stepMatch)
                                            controller.cancel();
                                        else
                                            controller.matchAndMoveSubtitles();
                                    }
                                }
                            }
                        }
                    }

                    // ── Step 3 ──
                    Rectangle {
                        id: step3Panel
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100
                        radius: 8
                        color: pal.SurfaceEx_cardBgAlt
                        border.color: pal.SurfaceEx_cardBorderLight
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors { topMargin: 2; leftMargin: 10; rightMargin: 10; bottomMargin: 10 }
                            spacing: 5

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                color: "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 0
                                    anchors.rightMargin: 0
                                    spacing: 5

                                    Label {
                                        id: step3Title
                                        text: "步骤3：合成视频+字幕"
                                        color: pal.LabelEx_titleText
                                        font.pixelSize: 14
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
                            }

                            Label {
                                id: step3Desc
                                text: "将字幕嵌入视频，生成带字幕的新视频文件"
                                color: pal.LabelEx_subtitleText
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 0
                                spacing: 5

                                // 空闲状态：执行按钮
                                CheckBoxEx {
                                    id: gpuCheckBox
                                    Layout.fillWidth: true
                                    text: "GPU加速"
                                    paletteGroup: "CheckBoxEx"
                                    checked: controller ? settings.gpuAccel : false
                                    font.pixelSize: 11
                                    enabled: !controller || controller.currentStep === controller.stepNone
                                    onCheckedChanged: {
                                        if (controller)
                                            settings.gpuAccel = checked;
                                    }
                                }

                                // 预约停止
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 0
                                    spacing: 6
                                    visible: controller && controller.currentStep === controller.stepMerge

                                    Label {
                                        id: step3CompleteLabel
                                        text: "再完成"
                                        color: pal.LabelEx_infoText
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
                                            if (controller && controller.isProcessing) {
                                                var val = currentValue;
                                                var shutdownVal = stopActionCombo.currentValue === "关机";
                                                if (typeof val === "number") {
                                                    controller.requestStopAfterCount(val, shutdownVal);
                                                } else {
                                                    controller.requestStopAfterCount(0, shutdownVal);
                                                }
                                            }
                                        }
                                        paletteGroup: "ComboBoxEx"
                                    }

                                    Label {
                                        text: "个后"
                                        color: pal.LabelEx_labelText
                                        font.pixelSize: 11
                                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                    }

                                    ComboBoxEx {
                                        id: stopActionCombo
                                        model: ["停止", "关机"]
                                        currentIndex: 0
                                        implicitWidth: 70
                                        font.pixelSize: 11
                                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                        onActivated: {
                                            if (controller && controller.isProcessing) {
                                                var val = stopAfterCombo.currentValue;
                                                var shutdownVal = currentValue === "关机";
                                                if (typeof val === "number") {
                                                    controller.requestStopAfterCount(val, shutdownVal);
                                                } else {
                                                    controller.requestStopAfterCount(0, shutdownVal);
                                                }
                                            }
                                        }
                                        paletteGroup: "ComboBoxEx"
                                    }
                                }

                                IconButton {
                                    id: step3Btn
                                    Layout.preferredWidth: 72
                                    text: controller && controller.currentStep === controller.stepMerge ? "停止" : "执行"
                                    tooltip: controller && controller.currentStep === controller.stepMerge ? "强制终止当前合成任务" : "合成视频+字幕"
                                    normalColor: controller && controller.currentStep === controller.stepMerge ? pal.CustomSubtitlePage_step3StopBtn_normalColor : pal.CustomSubtitlePage_step3ExecBtn_normalColor
                                    hoverColor: controller && controller.currentStep === controller.stepMerge ? pal.CustomSubtitlePage_step3StopBtn_hoverColor : pal.CustomSubtitlePage_step3ExecBtn_hoverColor
                                    borderColor: controller && controller.currentStep === controller.stepMerge ? pal.CustomSubtitlePage_step3StopBtn_borderColor : pal.CustomSubtitlePage_step3ExecBtn_borderColor
                                    textColor: controller && controller.currentStep === controller.stepMerge ? pal.CustomSubtitlePage_step3StopBtn_textColor : pal.CustomSubtitlePage_step3ExecBtn_textColor
                                    enabled: !controller || controller.currentStep === controller.stepNone || controller.currentStep === controller.stepMerge
                                    onClicked: {
                                        if (!controller) return;
                                        if (controller.currentStep === controller.stepMerge)
                                            controller.cancel();
                                        else
                                            controller.mergeSubtitleToVideo();
                                    }
                                }
                            }
                        }
                    }

                    // ── Step 4 ──
                    Rectangle {
                        id: step4Panel
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        radius: 8
                        color: pal.SurfaceEx_cardBgAlt
                        border.color: pal.SurfaceEx_cardBorderLight
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors { topMargin: 2; leftMargin: 10; rightMargin: 10; bottomMargin: 10 }
                            spacing: 5

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                color: "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 0
                                    anchors.rightMargin: 0
                                    spacing: 5

                                    Label {
                                        id: step4Title
                                        text: "步骤4：匹配+替换原视频"
                                        color: pal.LabelEx_titleText
                                        font.pixelSize: 14
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
                            }

                            Label {
                                id: step4Desc
                                text: "用合成后的视频替换原文件，并清理同名字幕。\n勾选\"名称弱匹配\"可按关键码匹配（如 aaa304 → aaa-304）"
                                color: pal.LabelEx_subtitleText
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 0
                                spacing: 5

                                CheckBoxEx {
                                    id: weakMatchCheckBox
                                    Layout.fillWidth: true
                                    text: "名称弱匹配"
                                    paletteGroup: "CheckBoxEx"
                                    checked: controller ? settings.weakMatch : false
                                    font.pixelSize: 11
                                    enabled: !controller || controller.currentStep === controller.stepNone
                                    onCheckedChanged: {
                                        if (controller)
                                            settings.weakMatch = checked;
                                    }
                                }

                                IconButton {
                                    id: step4Btn
                                    Layout.preferredWidth: 72
                                    text: controller && controller.currentStep === controller.stepReplace ? "停止" : "执行"
                                    tooltip: controller && controller.currentStep === controller.stepReplace ? "完成当前视频的备份、替换和清理字幕文件后停止" : "替换原视频"
                                    normalColor: controller && controller.currentStep === controller.stepReplace ? pal.CustomSubtitlePage_step3StopBtn_normalColor : pal.CustomSubtitlePage_step4ExecBtn_normalColor
                                    hoverColor: controller && controller.currentStep === controller.stepReplace ? pal.CustomSubtitlePage_step3StopBtn_hoverColor : pal.CustomSubtitlePage_step4ExecBtn_hoverColor
                                    borderColor: controller && controller.currentStep === controller.stepReplace ? pal.CustomSubtitlePage_step3StopBtn_borderColor : pal.CustomSubtitlePage_step4ExecBtn_borderColor
                                    textColor: controller && controller.currentStep === controller.stepReplace ? pal.CustomSubtitlePage_step3StopBtn_textColor : pal.CustomSubtitlePage_step4ExecBtn_textColor
                                    enabled: !controller || controller.currentStep === controller.stepNone || (controller.currentStep === controller.stepReplace && !controller.stopRequested)
                                    onClicked: {
                                        if (!controller) return;
                                        if (controller.currentStep === controller.stepReplace)
                                            controller.cancel();
                                        else
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
