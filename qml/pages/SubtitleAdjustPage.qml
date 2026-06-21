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
    property QtObject settings: pluginManager.getPluginSettings("subtitle-adjust")

    property bool hasVideo: controller && controller.currentVideoPath.length > 0
    property bool isSingleMode: controller ? settings.mode === 0 : true

    property int stepMs: 100
    property int maxOffsetMs: 60000 // 默认 ±1 分钟
    property string _lastLogLine: ""
    property string _currentSubtitleText: ""

    function fmtTime(ms) {
        if (ms < 0) return "-" + fmtTime(-ms)
        var h = Math.floor(ms / 3600000)
        var m = Math.floor((ms % 3600000) / 60000)
        var s = Math.floor((ms % 60000) / 1000)
        return (h < 10 ? "0" : "") + h + ":"
             + (m < 10 ? "0" : "") + m + ":"
             + (s < 10 ? "0" : "") + s
    }

    function fmtOffset(ms) {
        var sign = ms >= 0 ? "+" : "-"
        var absMs = Math.abs(ms)
        return sign + (absMs / 1000).toFixed(1) + "s"
    }

    function selectedFileUrl(url) {
        var p = url.toString()
        if (p.indexOf("file:///") === 0)
            return p.substring(8)
        if (p.indexOf("file://") === 0)
            return p.substring(7)
        return p
    }

    function tryStartAdjust(index) {
        if (!controller) return
        if (controller.isDirty) {
            unsavedDialog.targetIndex = index
            unsavedDialog.open()
        } else {
            controller.startAdjust(index)
        }
    }

    function player() { return videoDisplayLoader.item }

    padding: 0
    background: Rectangle {
        id: pageBg
        color: pal.SubtitleAdjustPage_pageBg_color
    }

    // ── File Dialogs ──
    FileDialog {
        id: videoFileDialog
        title: "选择视频文件"
        nameFilters: ["视频文件 (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.webm *.m4v *.ts)", "所有文件 (*)"]
        onAccepted: {
            if (controller) controller.videoPath = selectedFileUrl(selectedFile)
        }
    }

    FileDialog {
        id: subtitleFileDialog
        title: "选择字幕文件"
        nameFilters: ["字幕文件 (*.srt *.ass *.ssa)", "所有文件 (*)"]
        onAccepted: {
            if (controller) controller.subtitlePath = selectedFileUrl(selectedFile)
        }
    }

    FolderDialog {
        id: videoFolderDialog
        title: "选择视频文件夹"
        onAccepted: {
            if (controller) settings.videoFolder = selectedFileUrl(selectedFolder)
        }
    }

    FolderDialog {
        id: subtitleFolderDialog
        title: "选择字幕文件夹"
        onAccepted: {
            if (controller) settings.subtitleFolder = selectedFileUrl(selectedFolder)
        }
    }

    // ── Confirmation: unsaved changes ──
    Dialog {
        id: unsavedDialog
        title: "确认切换"
        modal: true
        anchors.centerIn: parent
        width: 420
        standardButtons: Dialog.NoButton
        closePolicy: Dialog.CloseOnEscape

        property int targetIndex: -1

        contentItem: ColumnLayout {
            spacing: 8
            Layout.margins: 4

            Label {
                id: unsavedMsg
                text: "当前字幕调整尚未导出，切换将丢失进度，是否继续？"
                color: pal.SubtitleAdjustPage_unsavedMsg_color
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
                    id: cancelBtn
                    text: "取消"
                    tooltip: "不切换，继续当前调整"
                    normalColor: pal.SubtitleAdjustPage_cancelBtn_normalColor
                    hoverColor: pal.SubtitleAdjustPage_cancelBtn_hoverColor
                    borderColor: pal.SubtitleAdjustPage_cancelBtn_borderColor
                    textColor: pal.SubtitleAdjustPage_cancelBtn_textColor
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: unsavedDialog.close()
                }

                IconButton {
                    id: continueBtn
                    text: "继续切换"
                    tooltip: "放弃当前调整，切换到选中项"
                    normalColor: pal.SubtitleAdjustPage_continueBtn_normalColor
                    hoverColor: pal.SubtitleAdjustPage_continueBtn_hoverColor
                    borderColor: pal.SubtitleAdjustPage_continueBtn_borderColor
                    textColor: pal.SubtitleAdjustPage_continueBtn_textColor
                    implicitWidth: 120
                    implicitHeight: 38
                    onClicked: {
                        var idx = unsavedDialog.targetIndex
                        unsavedDialog.close()
                        if (controller) {
                            controller.setOffsetMs(0)
                            controller.startAdjust(idx)
                        }
                    }
                }
            }
        }
    }

    // ── Controller signal connections ──
    Connections {
        target: controller
        function onLogMessage(message) {
            if (message.length === 0) return
            root._lastLogLine = message
        }
        function onExportFinished(success, message) {
            if (message.length === 0) return
            root._lastLogLine = message
        }
        function onVideoReady(videoPath, subtitlePath) {
            root._currentSubtitleText = ""
            videoDisplayLoader.active = true
            var p = videoDisplayLoader.item
            if (p) {
                p.source = "file:///" + videoPath
            }
        }
        function onCurrentVideoPathChanged() {
            if (!controller || controller.currentVideoPath.length === 0) {
                var p = videoDisplayLoader.item
                if (p) {
                    p.stop()
                    p.source = ""
                }
                videoDisplayLoader.active = false
            }
        }
    }

    // ── Subtitle polling timer ──
    Timer {
        id: subtitleTimer
        interval: 100
        repeat: true
        running: controller && player() && player().playbackState === 1
        onTriggered: {
            if (!controller || !player()) return
            root._currentSubtitleText = controller.getSubtitleTextAt(player().position)
        }
    }

    // ═══════════════ Main Layout ═══════════════
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 14

        // ═══════════════ Header ═══════════════
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
                    text: "字幕时间调整"
                    color: pal.SubtitleAdjustPage_titleLabel_color
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: descLabel
                    text: "实时调整字幕时间点以匹配视频"
                    color: pal.SubtitleAdjustPage_descLabel_color
                    font.pixelSize: 14
                }
            }
        }

        // ═══════════════ Top Panel: 模式 + 路径配置 ═══════════════
        Rectangle {
            id: topPanel
            Layout.fillWidth: true
            implicitHeight: topPanelContent.implicitHeight + 36
            radius: 10
            color: pal.SubtitleAdjustPage_topPanel_color
            border.color: pal.SubtitleAdjustPage_topPanel_borderColor
            border.width: 1

            ColumnLayout {
                id: topPanelContent
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                // Row 1: Mode Switch
                RowLayout {
                    spacing: 0
                    Layout.fillWidth: true

                    RadioButtonEx {
                        id: singleModeRadio
                        implicitWidth: 120
                        text: "单文件模式"
                        textColor: pal.radioButton_textColor
                        checked: isSingleMode
                        font.pixelSize: 13
                        onCheckedChanged: {
                            if (checked && controller && settings.mode !== 0)
                                settings.mode = 0
                        }
                    }

                    RadioButtonEx {
                        id: batchModeRadio
                        implicitWidth: 120
                        text: "批量处理模式"
                        textColor: pal.radioButton_textColor
                        checked: !isSingleMode
                        font.pixelSize: 13
                        onCheckedChanged: {
                            if (checked && controller && settings.mode !== 1)
                                settings.mode = 1
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // Row 2: Video Path
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: videoFileLabel
                        text: "视频文件路径"
                        color: pal.SubtitleAdjustPage_videoFileLabel_color
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        font.pixelSize: 11
                        text: isSingleMode
                            ? (controller ? controller.videoPath : "")
                            : (controller ? settings.videoFolder : "")
                        readOnly: true
                        placeholderText: isSingleMode ? "选择视频文件" : "选择视频文件夹"
                        clip: true
                    }

                    CheckBoxEx {
                        id: recursiveCheck1
                        implicitWidth: 110
                        text: "递归子文件夹"
                        textColor: pal.checkBox_textColor
                        font.pixelSize: 12
                        visible: !isSingleMode
                        checked: controller ? settings.recursiveVideo : false
                        onCheckedChanged: {
                            if (controller) settings.recursiveVideo = checked
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: isSingleMode ? "选择视频文件" : "选择视频文件夹"
                        onClicked: isSingleMode ? videoFileDialog.open() : videoFolderDialog.open()
                    }
                }

                // Row 3: Subtitle Path
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: subtitleFileLabel
                        text: "字幕文件路径"
                        color: pal.SubtitleAdjustPage_subtitleFileLabel_color
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        font.pixelSize: 11
                        text: isSingleMode
                            ? (controller ? controller.subtitlePath : "")
                            : (controller ? settings.subtitleFolder : "")
                        readOnly: true
                        placeholderText: isSingleMode ? "选择字幕文件" : "选择字幕文件夹"
                        clip: true
                    }

                    CheckBoxEx {
                        id: recursiveCheck2
                        implicitWidth: 110
                        text: "递归子文件夹"
                        textColor: pal.checkBox_textColor
                        font.pixelSize: 12
                        visible: !isSingleMode
                        checked: controller ? settings.recursiveSubtitle : false
                        onCheckedChanged: {
                            if (controller) settings.recursiveSubtitle = checked
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: isSingleMode ? "选择字幕文件" : "选择字幕文件夹"
                        onClicked: isSingleMode ? subtitleFileDialog.open() : subtitleFolderDialog.open()
                    }
                }

                // Row 4: 开始映射
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Item { Layout.fillWidth: true }

                    IconButton {
                        id: startMapBtn
                        implicitWidth: 150
                        text: "开始处理"
                        tooltip: isSingleMode ? "将单个文件加入列表" : "扫描文件夹并自动匹配视频与字幕文件"
                        normalColor: pal.SubtitleAdjustPage_startMapBtn_normalColor
                        hoverColor: pal.SubtitleAdjustPage_startMapBtn_hoverColor
                        borderColor: pal.SubtitleAdjustPage_startMapBtn_borderColor
                        textColor: pal.SubtitleAdjustPage_startMapBtn_textColor
                        enabled: {
                            if (!controller) return false
                            if (isSingleMode)
                                return controller.videoPath.length > 0 && controller.subtitlePath.length > 0
                            else
                                return settings.videoFolder.length > 0 && settings.subtitleFolder.length > 0
                        }
                        onClicked: {
                            if (controller) controller.startMatch()
                        }
                    }
                }
            }
        }

        // ═══════════════ Log Row + Shortcut Hints ═══════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            radius: 6
            color: pal.SubtitleAdjustPage_logBar_color

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        id: logText
                        Layout.fillWidth: true
                        text: root._lastLogLine.length > 0 ? root._lastLogLine : "就绪"
                        color: root._lastLogLine.length > 0 ? pal.SubtitleAdjustPage_logText_color_log : pal.SubtitleAdjustPage_logText_color_idle
                        font.pixelSize: 11
                        font.family: "Consolas, 'Courier New', monospace"
                        elide: Text.ElideRight
                    }
                }
            } 
        }

        // ═══════════════ Bottom: 3-Column Layout ═══════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            // ── Left: Mapping Table ──
            Rectangle {
                id: matchPanel
                Layout.preferredWidth: 360
                Layout.fillHeight: true
                radius: 10
                color: pal.SubtitleAdjustPage_matchPanel_color
                border.color: pal.SubtitleAdjustPage_matchPanel_borderColor
                border.width: 1
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 0

                    // Header + column labels
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        spacing: 0

                        Label {
                            id: numHeader
                            Layout.preferredWidth: 20
                            text: "#"
                            color: pal.SubtitleAdjustPage_numHeader_color
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Label {
                            id: videoHeader
                            Layout.preferredWidth: 128
                            text: "视频文件"
                            color: pal.SubtitleAdjustPage_videoHeader_color
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Label {
                            id: subHeader
                            Layout.preferredWidth: 128
                            text: "字幕文件"
                            color: pal.SubtitleAdjustPage_subHeader_color
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Label {
                            id: statusHeader
                            Layout.fillWidth: true
                            text: "状态"
                            color: pal.SubtitleAdjustPage_statusHeader_color
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    Rectangle {
                        id: headerSep
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: pal.SubtitleAdjustPage_headerSep_color
                    }

                    // Table body area
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            id: matchListView
                            anchors.fill: parent
                            model: controller ? controller.matchModel : null
                            visible: controller && controller.matchModel && controller.matchModel.count > 0
                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                            }
                            highlightMoveDuration: 0
                            highlightResizeDuration: 0

                            delegate: Rectangle {
                                id: delegateRoot
                                width: matchListView.width
                                height: 36
                                required property int index
                                required property string videoDisplay
                                required property string subtitleDisplay
                                required property string statusDisplay
                                required property int status

                                color: {
                                    if (status === 1) return pal.SubtitleAdjustPage_delegateRoot_bg_matched
                                    if (ListView.isCurrentItem) return pal.SubtitleAdjustPage_delegateRoot_bg_selected
                                    return index % 2 === 0 ? pal.SubtitleAdjustPage_delegateRoot_bg_even : pal.SubtitleAdjustPage_delegateRoot_bg_odd
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 0

                                    Label {
                                        id: indexLabel
                                        Layout.preferredWidth: 20
                                        text: delegateRoot.index + 1
                                        color: pal.SubtitleAdjustPage_indexLabel_color
                                        font.pixelSize: 11
                                    }

                                    Label {
                                        id: videoDisplayLabel
                                        Layout.preferredWidth: 128
                                        text: delegateRoot.videoDisplay
                                        color: pal.SubtitleAdjustPage_videoDisplayLabel_color
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        id: subtitleDisplayLabel
                                        Layout.preferredWidth: 128
                                        text: delegateRoot.subtitleDisplay
                                        color: pal.SubtitleAdjustPage_subtitleDisplayLabel_color
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        id: statusDisplayLabel
                                        Layout.fillWidth: true
                                        text: delegateRoot.statusDisplay
                                        color: delegateRoot.status === 1 ? pal.SubtitleAdjustPage_statusDisplayLabel_color_matched : pal.SubtitleAdjustPage_statusDisplayLabel_color_normal
                                        font.pixelSize: 12
                                        font.bold: delegateRoot.status === 1
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        matchListView.currentIndex = delegateRoot.index
                                        root.tryStartAdjust(delegateRoot.index)
                                    }
                                }
                            }
                        }

                        // Empty state
                        Label {
                            id: emptyMatchLabel
                            anchors.centerIn: parent
                            text: "暂无映射，请选择文件后点击「开始映射」"
                            color: pal.SubtitleAdjustPage_emptyMatchLabel_color
                            font.pixelSize: 13
                            visible: !controller || !controller.matchModel || controller.matchModel.count === 0
                        }
                    }
                }
            }

            // ── Middle: Video Preview ──
            Rectangle {
                id: videoPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: pal.SubtitleAdjustPage_videoPanel_color
                border.color: pal.SubtitleAdjustPage_videoPanel_borderColor
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // Video area
                    Rectangle {
                        id: videoBg
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6
                        color: hasVideo ? pal.SubtitleAdjustPage_videoBg_color_active : pal.SubtitleAdjustPage_videoBg_color_normal

                        // Empty state
                        Column {
                            anchors.centerIn: parent
                            spacing: 8
                            visible: !hasVideo

                            Label {
                                id: emptyVideoTitle
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "请在左侧映射列表中点击一项以开始调整"
                                color: pal.SubtitleAdjustPage_emptyVideoTitle_color
                                font.pixelSize: 15
                            }
                            Label {
                                id: emptyVideoDesc
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "加载后将在此处实时预览视频"
                                color: pal.SubtitleAdjustPage_emptyVideoDesc_color
                                font.pixelSize: 12
                            }
                        }

                        // Video output
                        Loader {
                            id: videoDisplayLoader
                            anchors.fill: parent
                            active: hasVideo
                            source: "../components/SubtitleAdjustVideoPlayer.qml"

                            onLoaded: {
                                item.source = "file:///" + (controller ? controller.currentVideoPath : "")
                            }
                        }

                        // 视频未播放时的提示
                        Rectangle {
                            id: videoOverlay
                            anchors.fill: parent
                            radius: 6
                            color: pal.SubtitleAdjustPage_videoOverlay_color
                            visible: {
                                var p = videoDisplayLoader.item
                                hasVideo && p && p.playbackState === 0
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: 8

                                Label {
                                    id: playPromptText
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请点击播放，开始调整字幕"
                                    color: pal.SubtitleAdjustPage_playPromptText_color
                                    font.pixelSize: 20
                                }
                            }
                        }

                        // Subtitle overlay
                        Rectangle {
                            id: subtitleOverlay
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 20
                            anchors.rightMargin: 20
                            anchors.bottomMargin: 20
                            height: subtitleText.implicitHeight + 20
                            radius: 8
                            color: pal.SubtitleAdjustPage_subtitleOverlay_color
                            visible: hasVideo

                            Label {
                                id: subtitleText
                                anchors.centerIn: parent
                                text: root._currentSubtitleText
                                color: pal.SubtitleAdjustPage_subtitleText_color
                                font.pixelSize: 20
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                            }
                        }
                    }

                    // Playback controls
                    Rectangle {
                        id: playbackControls
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        radius: 6
                        color: pal.SubtitleAdjustPage_playbackControls_color
                        border.color: pal.SubtitleAdjustPage_playbackControls_borderColor
                        border.width: 1
                        visible: hasVideo && videoDisplayLoader.active

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            IconButton {
                                id: seekBackBtn
                                implicitWidth: 26
                                iconSource: "qrc:/icons/video-seekdec.svg"
                                tooltip: "快退 5 秒"
                                normalColor: pal.SubtitleAdjustPage_seekBackBtn_normalColor
                                hoverColor: pal.SubtitleAdjustPage_seekBackBtn_hoverColor
                                borderColor: pal.SubtitleAdjustPage_seekBackBtn_borderColor
                                onClicked: {
                                    var p = videoDisplayLoader.item
                                    if (p) p.position = Math.max(0, p.position - 5000)
                                }
                            }

                            IconButton {
                                id: playBtn
                                implicitWidth: 32
                                property var p: videoDisplayLoader.item
                                property bool isPlaying: p && p.playbackState === 1
                                iconSource: isPlaying ? "qrc:/icons/video-pause.svg" : "qrc:/icons/video-play.svg"
                                tooltip: isPlaying ? "暂停" : "播放"
                                normalColor: pal.SubtitleAdjustPage_playBtn_normalColor
                                hoverColor: pal.SubtitleAdjustPage_playBtn_hoverColor
                                borderColor: pal.SubtitleAdjustPage_playBtn_borderColor
                                onClicked: {
                                    var p = videoDisplayLoader.item
                                    if (p) {
                                        if (p.playbackState === 1)
                                            p.pause()
                                        else
                                            p.play()
                                    }
                                }
                            }

                            IconButton {
                                id: seekFwdBtn
                                implicitWidth: 26
                                iconSource: "qrc:/icons/video-seekadd.svg"
                                tooltip: "快进 5 秒"
                                normalColor: pal.SubtitleAdjustPage_seekFwdBtn_normalColor
                                hoverColor: pal.SubtitleAdjustPage_seekFwdBtn_hoverColor
                                borderColor: pal.SubtitleAdjustPage_seekFwdBtn_borderColor
                                onClicked: {
                                    var p = videoDisplayLoader.item
                                    if (p) p.position = Math.min(p.duration, p.position + 5000)
                                }
                            }

                            Label {
                                id: positionLabel
                                text: {
                                    var p = videoDisplayLoader.item
                                    root.fmtTime(p ? p.position : 0)
                                }
                                color: pal.SubtitleAdjustPage_positionLabel_color
                                font.pixelSize: 12
                                font.family: "Consolas, monospace"
                                Layout.preferredWidth: 70
                            }

                            Slider {
                                id: seekSlider
                                Layout.fillWidth: true
                                property var p: videoDisplayLoader.item
                                from: 0
                                to: (p && p.duration > 0) ? p.duration : 1
                                value: p ? p.position : 0
                                enabled: p && p.duration > 0
                                onMoved: {
                                    var p = videoDisplayLoader.item
                                    if (p) p.position = value
                                }
                            }

                            Label {
                                id: durationLabel
                                text: {
                                    var p = videoDisplayLoader.item
                                    root.fmtTime(p ? p.duration : 0)
                                }
                                color: pal.SubtitleAdjustPage_durationLabel_color
                                font.pixelSize: 12
                                font.family: "Consolas, monospace"
                                Layout.preferredWidth: 70
                            }
                        }
                    }
                }
            }

            // ── Right: Adjustment Panel ──
            Rectangle {
                id: adjPanel
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                radius: 10
                color: pal.SubtitleAdjustPage_adjPanel_color
                border.color: pal.SubtitleAdjustPage_adjPanel_borderColor
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 10

                    // Max Offset value
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            id: offsetTitle
                            text: "偏移量"
                            color: pal.SubtitleAdjustPage_offsetTitle_color
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            id: maxOffsetTitle
                            text: "最大偏移"
                            color: pal.SubtitleAdjustPage_maxOffsetTitle_color
                            font.pixelSize: 11
                        }

                        ComboBoxEx {
                            id: maxOffsetCombo
                            model: [
                                { text: "30 秒", value: 30000 },
                                { text: "1 分钟", value: 60000 },
                                { text: "5 分钟", value: 300000 },
                                { text: "10 分钟", value: 600000 },
                            ]
                            textRole: "text"
                            valueRole: "value"
                            currentIndex: 1
                            font.pixelSize: 11
                            implicitWidth: 120
                            onActivated: {
                                root.maxOffsetMs = currentValue
                            }
                        }
                    }

                    // OffsetValue Text
                    Label {
                        id: offsetValue
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        text: controller ? root.fmtOffset(controller.offsetMs) : "±0.0s"
                        color: controller && controller.offsetMs !== 0 ? (controller.offsetMs > 0 ? pal.SubtitleAdjustPage_offsetValue_color_positive : pal.SubtitleAdjustPage_offsetValue_color_negative) : pal.SubtitleAdjustPage_offsetValue_color_neutral
                        font.pixelSize: 36
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    // Offset slider
                    Rectangle {
                        id: sliderPanel
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        color: pal.SubtitleAdjustPage_sliderPanel_color
                        radius: 6

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 2

                            Slider {
                                id: offsetSlider
                                Layout.fillWidth: true
                                from: -root.maxOffsetMs
                                to: root.maxOffsetMs
                                value: controller ? controller.offsetMs : 0
                                stepSize: root.stepMs
                                enabled: hasVideo
                                onMoved: {
                                    if (controller) controller.setOffsetMs(value)
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Label {
                                    id: sliderMinLabel
                                    text: "-" + (root.maxOffsetMs / 1000) + "s"
                                    color: pal.SubtitleAdjustPage_sliderMinLabel_color
                                    font.pixelSize: 10
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    id: sliderZeroLabel
                                    text: "0"
                                    color: pal.SubtitleAdjustPage_sliderZeroLabel_color
                                    font.pixelSize: 10
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    id: sliderMaxLabel
                                    text: "+" + (root.maxOffsetMs / 1000) + "s"
                                    color: pal.SubtitleAdjustPage_sliderMaxLabel_color
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }

                    // Quick adjust buttons
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        IconButton {
                            id: earlyBtn
                            text: "提前 " + (root.stepMs / 1000).toFixed(1) + "s"
                            tooltip: "字幕提前 " + root.stepMs + "ms"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            normalColor: pal.SubtitleAdjustPage_earlyBtn_normalColor
                            hoverColor: pal.SubtitleAdjustPage_earlyBtn_hoverColor
                            borderColor: pal.SubtitleAdjustPage_earlyBtn_borderColor
                            textColor: pal.SubtitleAdjustPage_earlyBtn_textColor
                            enabled: hasVideo
                            onClicked: {
                                if (controller) controller.shiftBackward(root.stepMs)
                            }
                        }

                        IconButton {
                            id: delayBtn
                            text: "推迟 " + (root.stepMs / 1000).toFixed(1) + "s"
                            tooltip: "字幕推迟 " + root.stepMs + "ms"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            normalColor: pal.SubtitleAdjustPage_delayBtn_normalColor
                            hoverColor: pal.SubtitleAdjustPage_delayBtn_hoverColor
                            borderColor: pal.SubtitleAdjustPage_delayBtn_borderColor
                            textColor: pal.SubtitleAdjustPage_delayBtn_textColor
                            enabled: hasVideo
                            onClicked: {
                                if (controller) controller.shiftForward(root.stepMs)
                            }
                        }
                    }

                    // Step size selector
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            id: stepSizeLabel
                            text: "步长"
                            color: pal.SubtitleAdjustPage_stepSizeLabel_color
                            font.pixelSize: 13
                            font.bold: true
                        }

                        ComboBoxEx {
                            id: stepCombo
                            Layout.fillWidth: true
                            model: ["0.1s", "0.5s", "1s", "5s"]
                            currentIndex: 0
                            onActivated: {
                                var steps = [100, 500, 1000, 5000]
                                root.stepMs = steps[currentIndex]
                            }
                        }
                    }

                    // Replace original srt
                    CheckBoxEx {
                        Layout.fillWidth: true
                        text: "替换原字幕文件"
                        textColor: pal.checkBox_textColor
                        font.pixelSize: 12
                        checked: controller ? settings.overwriteOriginal : false
                        onCheckedChanged: {
                            if (controller) settings.overwriteOriginal = checked
                        }
                    }

                    // Export srt
                    IconButton {
                        id: exportBtn
                        Layout.fillWidth: true
                        text: "导出字幕文件"
                        tooltip: controller && controller.overwriteOriginal
                            ? "替换原字幕文件（将覆盖原文件）"
                            : "导出为 _adjusted.srt 文件"
                        implicitHeight: 42
                        normalColor: pal.SubtitleAdjustPage_exportBtn_normalColor
                        hoverColor: pal.SubtitleAdjustPage_exportBtn_hoverColor
                        borderColor: pal.SubtitleAdjustPage_exportBtn_borderColor
                        textColor: pal.SubtitleAdjustPage_exportBtn_textColor
                        enabled: hasVideo && controller && controller.isDirty
                        onClicked: {
                            if (controller) controller.exportSubtitle()
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}


