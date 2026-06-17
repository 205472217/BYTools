import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

Pane {
    id: root

    signal backRequested
    property var controller: null

    property bool hasVideo: controller && controller.currentVideoPath.length > 0
    property bool isSingleMode: controller ? controller.mode === 0 : true

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

    Component.onDestruction: {
        if (controller && typeof controller.reset === 'function')
            controller.reset()
    }

    padding: 0
    background: Rectangle {
        color: "#f4f6f9"
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
            if (controller) controller.videoFolder = selectedFileUrl(selectedFolder)
        }
    }

    FolderDialog {
        id: subtitleFolderDialog
        title: "选择字幕文件夹"
        onAccepted: {
            if (controller) controller.subtitleFolder = selectedFileUrl(selectedFolder)
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
                text: "当前字幕调整尚未导出，切换将丢失进度，是否继续？"
                color: "#334155"
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
                    text: "取消"
                    tooltip: "不切换，继续当前调整"
                    normalColor: "#e2e8f0"
                    hoverColor: "#cbd5e1"
                    borderColor: "#cbd5e1"
                    textColor: "#475569"
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: unsavedDialog.close()
                }

                IconButton {
                    text: "继续切换"
                    tooltip: "放弃当前调整，切换到选中项"
                    normalColor: "#dc2626"
                    hoverColor: "#b91c1c"
                    borderColor: "#b91c1c"
                    textColor: "#ffffff"
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
        spacing: 18

        // ═══════════════ Header ═══════════════
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
                    text: "字幕时间调整"
                    color: "#111827"
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: "实时调整字幕时间点以匹配视频"
                    color: "#64748b"
                    font.pixelSize: 14
                }
            }
        }

        // ═══════════════ Top Panel: 模式 + 路径配置 ═══════════════
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: topPanelContent.implicitHeight + 36
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
                id: topPanelContent
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10

                // Row 1: Mode Switch
                RowLayout {
                    spacing: 0
                    Layout.fillWidth: true

                    RadioButton {
                        id: singleModeRadio
                        text: "单文件模式"
                        checked: isSingleMode
                        font.pixelSize: 13
                        onCheckedChanged: {
                            if (checked && controller && controller.mode !== 0)
                                controller.mode = 0
                        }
                    }

                    RadioButton {
                        id: batchModeRadio
                        text: "批量处理模式"
                        checked: !isSingleMode
                        font.pixelSize: 13
                        onCheckedChanged: {
                            if (checked && controller && controller.mode !== 1)
                                controller.mode = 1
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // Row 2: Video Path
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "视频文件"
                        color: "#475569"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 60
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        implicitHeight: 26
                        font.pixelSize: 11
                        text: isSingleMode
                            ? (controller ? controller.videoPath : "")
                            : (controller ? controller.videoFolder : "")
                        readOnly: true
                        placeholderText: isSingleMode ? "选择视频文件" : "选择视频文件夹"
                        clip: true
                    }

                    IconButton {
                        implicitWidth: 26
                        implicitHeight: 26
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: isSingleMode ? "选择视频文件" : "选择视频文件夹"
                        onClicked: isSingleMode ? videoFileDialog.open() : videoFolderDialog.open()
                    }

                    Item {
                        Layout.preferredWidth: 70
                        CheckBox {
                            anchors.centerIn: parent
                            text: "递归"
                            font.pixelSize: 12
                            visible: !isSingleMode
                            checked: controller ? controller.recursiveVideo : false
                            onCheckedChanged: {
                                if (controller) controller.recursiveVideo = checked
                            }
                        }
                    }

                    // 与第3行右侧按钮同宽
                    Item { Layout.preferredWidth: 100 }
                }

                // Row 3: Subtitle Path + 开始映射
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "字幕文件"
                        color: "#475569"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 60
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        implicitHeight: 26
                        font.pixelSize: 11
                        text: isSingleMode
                            ? (controller ? controller.subtitlePath : "")
                            : (controller ? controller.subtitleFolder : "")
                        readOnly: true
                        placeholderText: isSingleMode ? "选择字幕文件" : "选择字幕文件夹"
                        clip: true
                    }

                    IconButton {
                        implicitWidth: 26
                        implicitHeight: 26
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: isSingleMode ? "选择字幕文件" : "选择字幕文件夹"
                        onClicked: isSingleMode ? subtitleFileDialog.open() : subtitleFolderDialog.open()
                    }

                    Item {
                        Layout.preferredWidth: 70
                        CheckBox {
                            anchors.centerIn: parent
                            text: "递归"
                            font.pixelSize: 12
                            visible: !isSingleMode
                            checked: controller ? controller.recursiveSubtitle : false
                            onCheckedChanged: {
                                if (controller) controller.recursiveSubtitle = checked
                            }
                        }
                    }

                    IconButton {
                        text: "开始映射"
                        tooltip: isSingleMode ? "将单个文件加入映射列表" : "扫描文件夹并自动匹配视频与字幕文件"
                        implicitWidth: 100
                        implicitHeight: 30
                        normalColor: "#2563eb"
                        hoverColor: "#1d4ed8"
                        borderColor: "#1d4ed8"
                        textColor: "#ffffff"
                        enabled: {
                            if (!controller) return false
                            if (isSingleMode)
                                return controller.videoPath.length > 0 && controller.subtitlePath.length > 0
                            else
                                return controller.videoFolder.length > 0 && controller.subtitleFolder.length > 0
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
            Layout.preferredHeight: 28
            radius: 6
            color: "#f1f5f9"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    text: root._lastLogLine.length > 0 ? root._lastLogLine : "就绪"
                    color: root._lastLogLine.length > 0 ? "#334155" : "#94a3b8"
                    font.pixelSize: 11
                    font.family: "Consolas, 'Courier New', monospace"
                    elide: Text.ElideRight
                }

                Label {
                    text: "← → 偏移 ±" + (root.stepMs / 1000).toFixed(1) + "s  |  Ctrl+←/→ ±500ms  |  Space 播放/暂停  |  A/D 快退/快进 5s"
                    color: "#94a3b8"
                    font.pixelSize: 10
                    visible: hasVideo && videoDisplayLoader.active
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
                Layout.preferredWidth: 360
                Layout.fillHeight: true
                radius: 10
                color: "#ffffff"
                border.color: "#e5e9f0"
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
                            Layout.preferredWidth: 140
                            text: "视频文件"
                            color: "#64748b"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Label {
                            Layout.preferredWidth: 140
                            text: "字幕文件"
                            color: "#64748b"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "状态"
                            color: "#64748b"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#e2e8f0"
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
                                    if (status === 1) return "#dcfce7"
                                    if (ListView.isCurrentItem) return "#eff6ff"
                                    return index % 2 === 0 ? "#ffffff" : "#fafbfc"
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 0

                                    Label {
                                        Layout.preferredWidth: 140
                                        text: delegateRoot.videoDisplay
                                        color: "#334155"
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        Layout.preferredWidth: 140
                                        text: delegateRoot.subtitleDisplay
                                        color: "#334155"
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: delegateRoot.statusDisplay
                                        color: delegateRoot.status === 1 ? "#059669" : "#94a3b8"
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
                            anchors.centerIn: parent
                            text: "暂无映射，请选择文件后点击「开始映射」"
                            color: "#94a3b8"
                            font.pixelSize: 13
                            visible: !controller || !controller.matchModel || controller.matchModel.count === 0
                        }
                    }
                }
            }

            // ── Middle: Video Preview ──
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#ffffff"
                border.color: "#e5e9f0"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // Video area
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6
                        color: hasVideo ? "#000000" : "#f1f5f9"

                        // Empty state
                        Column {
                            anchors.centerIn: parent
                            spacing: 8
                            visible: !hasVideo

                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "请在左侧映射列表中点击一项以开始调整"
                                color: "#94a3b8"
                                font.pixelSize: 15
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "加载后将在此处实时预览视频"
                                color: "#c7d2e0"
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
                            anchors.fill: parent
                            radius: 6
                            color: "#000000"
                            visible: {
                                var p = videoDisplayLoader.item
                                hasVideo && p && p.playbackState === 0
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: 8

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请点击播放，开始调整字幕"
                                    color: "#e2e8f0"
                                    font.pixelSize: 16
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "使用 ←/→ 调整字幕偏移，A/D 快退/快进视频"
                                    color: "#94a3b8"
                                    font.pixelSize: 12
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
                            color: "#C0000000"
                            visible: hasVideo

                            Label {
                                id: subtitleText
                                anchors.centerIn: parent
                                text: root._currentSubtitleText
                                color: "#ffffff"
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
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"
                        border.width: 1
                        visible: hasVideo && videoDisplayLoader.active

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            IconButton {
                                implicitWidth: 26
                                implicitHeight: 26
                                iconSource: "qrc:/icons/video-seekdec.svg"
                                tooltip: "快退 5 秒"
                                normalColor: "#ffffff"
                                hoverColor: "#f1f5f9"
                                borderColor: "#cbd5e1"
                                onClicked: {
                                    var p = videoDisplayLoader.item
                                    if (p) p.position = Math.max(0, p.position - 5000)
                                }
                            }

                            IconButton {
                                id: playBtn
                                implicitWidth: 32
                                implicitHeight: 32
                                property var p: videoDisplayLoader.item
                                property bool isPlaying: p && p.playbackState === 1
                                iconSource: isPlaying ? "qrc:/icons/video-pause.svg" : "qrc:/icons/video-play.svg"
                                tooltip: isPlaying ? "暂停" : "播放"
                                normalColor: "#ffffff"
                                hoverColor: "#f1f5f9"
                                borderColor: "#cbd5e1"
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
                                implicitWidth: 26
                                implicitHeight: 26
                                iconSource: "qrc:/icons/video-seekadd.svg"
                                tooltip: "快进 5 秒"
                                normalColor: "#ffffff"
                                hoverColor: "#f1f5f9"
                                borderColor: "#cbd5e1"
                                onClicked: {
                                    var p = videoDisplayLoader.item
                                    if (p) p.position = Math.min(p.duration, p.position + 5000)
                                }
                            }

                            Label {
                                text: {
                                    var p = videoDisplayLoader.item
                                    root.fmtTime(p ? p.position : 0)
                                }
                                color: "#64748b"
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
                                text: {
                                    var p = videoDisplayLoader.item
                                    root.fmtTime(p ? p.duration : 0)
                                }
                                color: "#64748b"
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
                    anchors.margins: 18
                    spacing: 0

                    // 最大偏移选择
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 8

                        Label {
                            text: "偏移量"
                            color: "#64748b"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: "最大偏移"
                            color: "#64748b"
                            font.pixelSize: 11
                        }

                        ComboBox {
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
                            implicitHeight: 26
                            onActivated: {
                                root.maxOffsetMs = currentValue
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        text: controller ? root.fmtOffset(controller.offsetMs) : "±0.0s"
                        color: controller && controller.offsetMs !== 0 ? (controller.offsetMs > 0 ? "#059669" : "#dc2626") : "#64748b"
                        font.pixelSize: 36
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    // Offset slider
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.preferredHeight: 40
                        color: "#f8fafc"
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
                                    text: "-" + (root.maxOffsetMs / 1000) + "s"
                                    color: "#94a3b8"
                                    font.pixelSize: 10
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    text: "0"
                                    color: "#94a3b8"
                                    font.pixelSize: 10
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    text: "+" + (root.maxOffsetMs / 1000) + "s"
                                    color: "#94a3b8"
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }

                    // Quick adjust buttons
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        spacing: 12

                        IconButton {
                            text: "提前 " + (root.stepMs / 1000).toFixed(1) + "s"
                            tooltip: "字幕提前 " + root.stepMs + "ms"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            normalColor: "#fee2e2"
                            hoverColor: "#fecaca"
                            borderColor: "#fca5a5"
                            textColor: "#dc2626"
                            enabled: hasVideo
                            onClicked: {
                                if (controller) controller.shiftBackward(root.stepMs)
                            }
                        }

                        IconButton {
                            text: "推迟 " + (root.stepMs / 1000).toFixed(1) + "s"
                            tooltip: "字幕推迟 " + root.stepMs + "ms"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            normalColor: "#dcfce7"
                            hoverColor: "#bbf7d0"
                            borderColor: "#86efac"
                            textColor: "#059669"
                            enabled: hasVideo
                            onClicked: {
                                if (controller) controller.shiftForward(root.stepMs)
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        Layout.topMargin: 16
                        color: "#e2e8f0"
                    }

                    // Step size selector
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "步长"
                            color: "#475569"
                            font.pixelSize: 13
                            font.bold: true
                        }

                        ComboBoxEx {
                            id: stepCombo
                            Layout.fillWidth: true
                            model: ["100ms", "500ms", "1000ms", "5000ms"]
                            currentIndex: 0
                            onActivated: {
                                var steps = [100, 500, 1000, 5000]
                                root.stepMs = steps[currentIndex]
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#e2e8f0"
                    }

                    CheckBox {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        text: "替换原字幕文件"
                        font.pixelSize: 12
                        checked: controller ? controller.overwriteOriginal : false
                        onCheckedChanged: {
                            if (controller) controller.overwriteOriginal = checked
                        }
                    }

                    IconButton {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        Layout.bottomMargin: 8
                        text: "导出字幕文件"
                        tooltip: controller && controller.overwriteOriginal
                            ? "替换原字幕文件（将覆盖原文件）"
                            : "导出为 _adjusted.srt 文件"
                        implicitHeight: 42
                        normalColor: "#2563eb"
                        hoverColor: "#1d4ed8"
                        borderColor: "#1d4ed8"
                        textColor: "#ffffff"
                        enabled: hasVideo && controller && controller.isDirty
                        onClicked: {
                            if (controller) controller.exportSubtitle()
                        }
                    }
                }
            }
        }

    }

    // ── Keyboard shortcuts ──
    Keys.onLeftPressed: {
        if (hasVideo && controller) {
            if (event.modifiers & Qt.ControlModifier)
                controller.shiftBackward(500)
            else
                controller.shiftBackward(root.stepMs)
        }
    }

    Keys.onRightPressed: {
        if (hasVideo && controller) {
            if (event.modifiers & Qt.ControlModifier)
                controller.shiftForward(500)
            else
                controller.shiftForward(root.stepMs)
        }
    }

    Keys.onPressed: {
        if (hasVideo) {
            var p = videoDisplayLoader.item
            if (event.key === Qt.Key_A) {
                if (p) p.position = Math.max(0, p.position - 5000)
                event.accepted = true
            } else if (event.key === Qt.Key_D) {
                if (p) p.position = Math.min(p.duration, p.position + 5000)
                event.accepted = true
            }
        }
    }

    Keys.onSpacePressed: {
        if (hasVideo) {
            var p = videoDisplayLoader.item
            if (p) {
                if (p.playbackState === 1)
                    p.pause()
                else
                    p.play()
            }
        }
    }
}
