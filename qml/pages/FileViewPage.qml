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

    property bool hasFiles: controller && controller.fileCount > 0
    property bool hasSelection: controller && controller.currentFilePath.length > 0
    property string _lastLogLine: ""
    property var _typeNames: ["视频", "音频", "图片"]

    // ── mpv 检测（启动时已解压，此处仅检查文件是否存在） ──
    property bool _mpvAvailable: {
        if (!controller) return false
        var dir = pluginManager.pluginDirectory("file-view")
        return dir.length > 0 && pluginManager.fileExists(dir + "/mpv/mpv.exe")
    }
    property string _mpvExePath: _mpvAvailable
        ? pluginManager.pluginDirectory("file-view") + "/mpv/mpv.exe" : ""

    function safeInfo(key, fallback) {
        if (!controller || !hasSelection) return fallback || "-"
        var v = controller.currentFileInfo[key]
        return v !== undefined ? v : (fallback || "-")
    }

    function fmtSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
        if (bytes < 1073741824) return (bytes / 1048576).toFixed(1) + " MB"
        return (bytes / 1073741824).toFixed(2) + " GB"
    }

    function _hideNativeOverlay() {
        var p = videoPreviewLoader.item
        if (p && p.setNativeOverlayVisible)
            p.setNativeOverlayVisible(false)
    }
    function _showNativeOverlay() {
        var p = videoPreviewLoader.item
        if (p && p.setNativeOverlayVisible)
            p.setNativeOverlayVisible(true)
    }

    function selectedFileUrl(url) {
        var p = url.toString()
        if (p.indexOf("file:///") === 0)
            return p.substring(8)
        if (p.indexOf("file://") === 0)
            return p.substring(7)
        return p
    }

    padding: 0
    background: Rectangle {
        id: pageBg
        color: pal.SurfaceEx_pageBg
    }

    // ── File Dialog ──
    FolderDialog {
        id: folderDialog
        title: "选择源文件夹"
        onAccepted: {
            if (controller) controller.sourceFolder = selectedFileUrl(selectedFolder)
        }
    }

    // ── Controller signal connections ──
    Connections {
        target: controller
        function onLogMessage(message) {
            if (message.length === 0) return
            root._lastLogLine = message
        }
        function onScanFinished() {
            if (controller && controller.fileCount > 0)
                root._lastLogLine = "扫描完成，共 " + controller.fileCount + " 个文件"
        }
        function onCurrentModelIndexChanged() {
            if (controller)
                fileListView.currentIndex = controller.currentModelIndex
        }
    }

    function navPrevFile() {
        if (!controller) return
        var idx = controller.currentModelIndex
        if (idx > 0) controller.selectFile(idx - 1)
    }

    function navNextFile() {
        if (!controller) return
        var idx = controller.currentModelIndex
        if (idx < controller.fileCount - 1) controller.selectFile(idx + 1)
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
                paletteGroup: "IconBtnEx"
                onClicked: {
                    if (controller) controller.cleanTrash()
                    root._hideNativeOverlay()
                    root.backRequested()
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    id: titleLabel
                    text: "文件浏览器"
                    color: pal.LabelEx_titleText
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: descLabel
                    text: "浏览指定文件夹下的任意类型文件"
                    color: pal.LabelEx_subtitleText
                    font.pixelSize: 14
                }
            }
        }

        // ═══════════════ Top Panel: 类型 + 路径 ═══════════════
        Rectangle {
            id: topPanel
            Layout.fillWidth: true
            implicitHeight: topPanelContent.implicitHeight + 36
            radius: 10
            color: pal.SurfaceEx_cardBg
            border.color: pal.SurfaceEx_cardBorder
            border.width: 1

            ColumnLayout {
                id: topPanelContent
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                // Row 1: File type radio
                RowLayout {
                    spacing: 0
                    Layout.fillWidth: true

                    Repeater {
                        model: root._typeNames

                        RadioButtonEx {
                            required property int index
                            required property string modelData

                            implicitWidth: 100
                            text: modelData
                            paletteGroup: "RadioButtonEx"
                            checked: controller ? controller.fileType === index : index === 0
                            font.pixelSize: 13
                            onCheckedChanged: {
                                if (checked && controller && controller.fileType !== index)
                                    controller.fileType = index
                            }
                        }
                            }
                        }

                // Row 2: Source folder
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: sourceFolderLabel
                        text: "源文件夹路径"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        font.pixelSize: 11
                        text: controller ? controller.sourceFolder : ""
                        readOnly: true
                        placeholderText: "选择源文件夹"
                        clip: true
                        paletteGroup: "TextFieldEx"
                    }

                    CheckBoxEx {
                        id: recursiveCheck
                        implicitWidth: 110
                        text: "递归子文件夹"
                        paletteGroup: "CheckBoxEx"
                        font.pixelSize: 12
                        checked: controller ? controller.recursive : false
                        onCheckedChanged: {
                            if (controller) controller.recursive = checked
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择源文件夹"
                        paletteGroup: "IconBtnEx"
                        onClicked: folderDialog.open()
                    }
                }

                // Row 3: Start scan button
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Item { Layout.fillWidth: true }

                    IconButton {
                        id: startScanBtn
                        implicitWidth: 150
                        text: "开始浏览"
                        tooltip: "扫描所选文件夹中的文件"
                        paletteGroup: "FileViewPage_scanBtn"
                        enabled: controller && controller.sourceFolder.length > 0
                        onClicked: {
                            if (!controller) return
                            controller.startScan()
                        }
                    }
                }
            }
        }

        // ═══════════════ Log Row ═══════════════
        Rectangle {
            id: logBar
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            radius: 6
            color: pal.SurfaceEx_statusBar

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6

                Label {
                    id: logText
                    Layout.fillWidth: true
                    text: root._lastLogLine.length > 0 ? root._lastLogLine : "就绪"
                    color: pal.LabelEx_statusText
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
            }
        }

        // ═══════════════ Bottom: 3-Column Layout ═══════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            // ── Left: File List ──
            Rectangle {
                id: fileListPanel
                Layout.preferredWidth: 320
                Layout.fillHeight: true
                radius: 10
                color: pal.SurfaceEx_cardBg
                border.color: pal.SurfaceEx_cardBorder
                border.width: 1
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 5

                    // Sort bar
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        radius: 6
                        color: pal.SurfaceEx_headerRowBg
                        border.color: pal.SurfaceEx_cardBorderLight
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 8
                            spacing: 6

                            Row {
                                spacing: 1

                                IconButton {
                                    implicitWidth: 24
                                    implicitHeight: 24
                                    iconSource: "qrc:/icons/to-top.svg"
                                    tooltip: "定位到顶部"
                                    normalColor: "transparent"
                                    hoverColor: pal.IconBtnEx_hoverColor
                                    pressColor: pal.IconBtnEx_pressColor
                                    borderColor: "transparent"
                                    defaultBorderColor: "transparent"
                                    textColor: pal.IconBtnEx_textColor
                                    onClicked: {
                                        fileListView.positionViewAtBeginning()
                                    }
                                }

                                IconButton {
                                    implicitWidth: 24
                                    implicitHeight: 24
                                    iconSource: "qrc:/icons/to-current.svg"
                                    tooltip: "定位到当前播放文件"
                                    normalColor: "transparent"
                                    hoverColor: pal.IconBtnEx_hoverColor
                                    pressColor: pal.IconBtnEx_pressColor
                                    borderColor: "transparent"
                                    defaultBorderColor: "transparent"
                                    textColor: pal.IconBtnEx_textColor
                                    onClicked: {
                                        if (fileListView.currentIndex >= 0)
                                            fileListView.positionViewAtIndex(fileListView.currentIndex, ListView.Contain)
                                    }
                                }

                                IconButton {
                                    implicitWidth: 24
                                    implicitHeight: 24
                                    iconSource: "qrc:/icons/to-bottom.svg"
                                    tooltip: "定位到底部"
                                    normalColor: "transparent"
                                    hoverColor: pal.IconBtnEx_hoverColor
                                    pressColor: pal.IconBtnEx_pressColor
                                    borderColor: "transparent"
                                    defaultBorderColor: "transparent"
                                    textColor: pal.IconBtnEx_textColor
                                    onClicked: {
                                        fileListView.positionViewAtEnd()
                                    }
                                }
                            }

                            Label {
                                text: "排序:"
                                color: pal.LabelEx_labelText
                                font.pixelSize: 11
                            }

                            ComboBoxEx {
                                id: sortCombo
                                Layout.fillWidth: true
                                implicitHeight: 24
                                model: ["文件名", "修改时间", "创建时间", "文件大小", "类型"]
                                currentIndex: controller ? controller.sortField : 0
                                font.pixelSize: 11
                                onActivated: {
                                    if (controller) controller.sortField = currentIndex
                                }
                                paletteGroup: "ComboBoxEx"
                            }

                            IconButton {
                                id: sortOrderBtn
                                implicitWidth: 28
                                implicitHeight: 24
                                text: controller && controller.sortAscending ? "▲" : "▼"
                                tooltip: controller && controller.sortAscending ? "升序" : "降序"
                                normalColor: "transparent"
                                hoverColor: pal.IconBtnEx_hoverColor
                                pressColor: pal.IconBtnEx_pressColor
                                borderColor: "transparent"
                                defaultBorderColor: "transparent"
                                textColor: pal.IconBtnEx_textColor
                                onClicked: {
                                    if (controller) controller.sortAscending = !controller.sortAscending
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: listHeaderSep
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: pal.SurfaceEx_divider
                    }

                    // List body
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            id: fileListView
                            anchors.fill: parent
                            model: controller ? controller.fileListModel : null
                            visible: hasFiles
                            currentIndex: -1
                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                            }
                            highlightMoveDuration: 0
                            highlightResizeDuration: 0

                            delegate: Rectangle {
                                id: fileDelegate
                                width: fileListView.width
                                height: 48
                                required property int index
                                required property string fileName
                                required property string fileSizeDisplay
                                required property string modifiedTimeDisplay
                                required property string fileType
                                required property int typeCategory
                                required property bool fileDeleted

                                property bool rowHovered: false

                                color: {
                                    if (ListView.isCurrentItem) return pal.SurfaceEx_rowSelectedBg
                                    return index % 2 === 0 ? pal.SurfaceEx_rowEvenBg : pal.SurfaceEx_rowOddBg
                                }

                                RowLayout {
                                    id: rowContent
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 44
                                    spacing: 6
                                    z: 0

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 6

                                            Label {
                                                id: indexLabel
                                                text: fileDelegate.index + 1
                                                color: pal.LabelEx_infoText
                                                font.pixelSize: 11
                                                Layout.preferredWidth: 28
                                            }

                                            Label {
                                                id: nameLabel
                                                Layout.fillWidth: true
                                                text: fileDelegate.fileName
                                                color: fileDelegate.fileDeleted ? pal.LabelEx_deleteColor : pal.LabelEx_valueText
                                                font.pixelSize: 12
                                                elide: Text.ElideRight
                                                font.strikeout: fileDelegate.fileDeleted
                                                font.underline: fileDelegate.fileDeleted
                                            }

                                            Rectangle {
                                                id: typeTag
                                                width: typeTagText.implicitWidth + 10
                                                height: 18
                                                radius: 3
                                                color: {
                                                    switch (fileDelegate.typeCategory) {
                                                    case 0: return pal.LabelEx_Video_BgRect
                                                    case 1: return pal.LabelEx_Audio_BgRect
                                                    case 2: return pal.LabelEx_Image_BgRect
                                                    default: return pal.LabelEx_Other_BgRect
                                                    }
                                                }

                                                Label {
                                                    id: typeTagText
                                                    anchors.centerIn: parent
                                                    text: fileDelegate.fileType
                                                    color: {
                                                        switch (fileDelegate.typeCategory) {
                                                        case 0: return pal.LabelEx_Video_Text
                                                        case 1: return pal.LabelEx_Audio_Text
                                                        case 2: return pal.LabelEx_Image_Text
                                                        default: return pal.LabelEx_Other_Text
                                                        }
                                                    }
                                                    font.pixelSize: 10
                                                    font.bold: true
                                                }
                                            }
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 12

                                            Item { Layout.preferredWidth: 28 }

                                            Label {
                                                text: fileDelegate.fileSizeDisplay
                                                color: pal.LabelEx_infoText
                                                font.pixelSize: 10
                                                elide: Text.ElideRight
                                            }

                                            Item { Layout.fillWidth: true }

                                            Label {
                                                text: fileDelegate.modifiedTimeDisplay
                                                color: pal.LabelEx_infoText
                                                font.pixelSize: 10
                                                elide: Text.ElideRight
                                            }
                                        }
                                    }
                                }

                                MouseArea {
                                    id: rowMouse
                                    anchors.fill: parent
                                    anchors.rightMargin: 36
                                    z: 0
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onEntered: fileDelegate.rowHovered = true
                                    onExited: fileDelegate.rowHovered = false
                                    onClicked: {
                                        fileListView.currentIndex = fileDelegate.index
                                        if (controller) controller.selectFile(fileDelegate.index)
                                    }
                                }

                                Rectangle {
                                    id: deleteBtn
                                    anchors.right: parent.right
                                    anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 28
                                    height: 28
                                    radius: 6
                                    z: 1
                                    visible: fileDelegate.fileDeleted
                                            || fileListView.currentIndex === fileDelegate.index
                                            || fileDelegate.rowHovered
                                            || delMouse.containsMouse

                                    color: delMouse.containsMouse
                                        ? (fileDelegate.fileDeleted
                                            ? pal.IconBtnEx_hoverColor
                                            : pal.IconBtnEx_deleteBgColor)
                                        : "transparent"
                                    border.width: delMouse.containsMouse ? 1 : 0
                                    border.color: fileDelegate.fileDeleted
                                        ? pal.SurfaceEx_cardBorderLight
                                        : (delMouse.containsMouse ? pal.LabelEx_deleteColor : "transparent")

                                    Label {
                                        anchors.centerIn: parent
                                        text: fileDelegate.fileDeleted ? "↩" : "✕"
                                        color: fileDelegate.fileDeleted
                                            ? pal.LabelEx_infoText
                                            : (delMouse.containsMouse ? pal.LabelEx_deleteColor : pal.LabelEx_infoText)

                                        ToolTip {
                                            visible: delMouse.containsMouse
                                            text: fileDelegate.fileDeleted ? "还原（到原始位置）" : "删除（到回收站）"
                                            delay: 300
                                        }
                                        font.pixelSize: 14
                                        font.bold: true
                                    }

                                    MouseArea {
                                        id: delMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onEntered: fileDelegate.rowHovered = true
                                        onExited: fileDelegate.rowHovered = false
                                        onClicked: {
                                            if (!controller) return
                                            if (fileDelegate.fileDeleted) {
                                                controller.restoreFile(fileDelegate.index)
                                            } else {
                                                if (fileDelegate.index === controller.currentModelIndex
                                                        && videoPreviewLoader.item) {
                                                    videoPreviewLoader.item.stop()
                                                    videoPreviewLoader.item.source = ""
                                                }
                                                var ok = controller.deleteFile(fileDelegate.index)
                                                if (!ok && fileDelegate.index === controller.currentModelIndex) {
                                                    root._lastLogLine = "删除失败：文件被占用，请先切换到其他文件再试"
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Empty state
                        Label {
                            id: emptyListLabel
                            anchors.centerIn: parent
                            text: "暂无文件，请选择文件夹后点击「开始浏览」"
                            color: pal.LabelEx_infoText
                            font.pixelSize: 13
                            visible: !hasFiles
                        }
                    }
                }
            }

            // ── Middle: Preview ──
            Rectangle {
                id: previewPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: pal.SurfaceEx_cardBg
                border.color: pal.SurfaceEx_cardBorder
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 8

                    // Preview area
                    Rectangle {
                        id: previewArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6
                        color: hasSelection ? pal.SurfaceEx_videoBg_active : pal.SurfaceEx_videoBg_normal
                        clip: true

                        // Empty state
                        Column {
                            anchors.centerIn: parent
                            spacing: 8
                            visible: !hasSelection

                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "请在左侧文件列表中点击一项以预览"
                                color: pal.LabelEx_infoText
                                font.pixelSize: 15
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "支持视频、音频和图片文件"
                                color: pal.LabelEx_infoText
                                font.pixelSize: 12
                            }
                        }

                        // ── Video/Audio preview ──
                        Loader {
                            id: videoPreviewLoader
                            anchors.fill: parent
                            active: hasSelection && controller
                                    && (controller.currentFileInfo.typeCategory === 0
                                        || controller.currentFileInfo.typeCategory === 1)
                            source: root._mpvAvailable
                                ? "../components/MpvViewer.qml"
                                : "../components/MediaViewer.qml"

                            onLoaded: {
                                if (controller) {
                                    if (root._mpvAvailable)
                                        item.mpvPath = root._mpvExePath
                                    item.controlsPaletteGroup = "VideoPlayerControls"
                                    item.source = "file:///" + controller.currentFilePath
                                    if (settings) {
                                        item.volume = settings.volume
                                        item.muted = settings.muted
                                        item.seekStepMs = settings.seekStepMs
                                        if (item.volume < 1)
                                            item.muted = true
                                    }
                                    item.play()
                                    item.previousRequested.connect(function() {
                                        if (!controller || controller.fileCount === 0) return
                                        var idx = controller.currentModelIndex
                                        controller.selectFile(idx > 0 ? idx - 1 : controller.fileCount - 1)
                                    })
                                    item.nextRequested.connect(function() {
                                        if (!controller || controller.fileCount === 0) return
                                        var idx = controller.currentModelIndex
                                        controller.selectFile(idx < controller.fileCount - 1 ? idx + 1 : 0)
                                    })
                                    item.deleteRequested.connect(function() {
                                        if (!controller || controller.fileCount === 0) return
                                        item.stop()
                                        item.source = ""
                                        controller.deleteFile(controller.currentModelIndex)
                                    })
                                }
                            }

                            Connections {
                                target: controller
                                function onCurrentFilePathChanged() {
                                    var p = videoPreviewLoader.item
                                    if (p && controller) {
                                        var newSrc = "file:///" + controller.currentFilePath
                                        if (p.source !== newSrc) {
                                            p.source = newSrc
                                            p.play()
                                        }
                                    }
                                }
                            }

                            Connections {
                                target: videoPreviewLoader.item
                                function onVolumeChanged() {
                                    if (settings && videoPreviewLoader.item)
                                        settings.volume = videoPreviewLoader.item.volume
                                }
                                function onMutedChanged() {
                                    if (settings && videoPreviewLoader.item)
                                        settings.muted = videoPreviewLoader.item.muted
                                }
                                function onSeekStepMsChanged() {
                                    if (settings && videoPreviewLoader.item)
                                        settings.seekStepMs = videoPreviewLoader.item.seekStepMs
                                }
                            }
                        }

                        // ── Image preview (Loader to avoid decode errors on non‑image files) ──
                        Loader {
                            id: imagePreviewLoader
                            anchors.fill: parent
                            active: hasSelection && controller
                                    && controller.currentFileInfo.typeCategory === 2
                            source: "../components/ImageViewer.qml"

                            onLoaded: {
                                if (!controller) return
                                item.source = controller.currentFilePath
                                item.hasPrevious = controller.currentModelIndex > 0
                                item.hasNext = controller.currentModelIndex < controller.fileCount - 1
                                item.previousRequested.connect(function() { navPrevFile() })
                                item.nextRequested.connect(function() { navNextFile() })
                                item.deleteRequested.connect(function() {
                                    if (!controller || controller.fileCount === 0) return
                                    controller.deleteFile(controller.currentModelIndex)
                                })
                            }

                            Connections {
                                target: controller
                                function onCurrentFilePathChanged() {
                                    var p = imagePreviewLoader.item
                                    if (p && controller)
                                        p.source = controller.currentFilePath
                                }
                                function onCurrentModelIndexChanged() {
                                    var p = imagePreviewLoader.item
                                    if (p && controller) {
                                        p.hasPrevious = controller.currentModelIndex > 0
                                        p.hasNext = controller.currentModelIndex < controller.fileCount - 1
                                    }
                                }
                            }
                        }



                    }
                }

            }

            // ── Right: Properties ──
            Rectangle {
                id: propPanel
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                radius: 10
                color: pal.SurfaceEx_cardBg
                border.color: pal.SurfaceEx_cardBorder
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 10

                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentHeight: propContent.implicitHeight
                        clip: true

                        ColumnLayout {
                            id: propContent
                            width: parent.width
                            spacing: 10

                            // File name
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: "文件名"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: hasSelection ? controller.currentFileInfo.fileName : "-"
                                    color: pal.LabelEx_valueText
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                    elide: Text.ElideRight
                                    maximumLineCount: 3
                                }
                            }

                            // File size
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: "文件大小"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                }
                                Label {
                                    text: hasSelection ? controller.currentFileInfo.fileSizeDisplay : "-"
                                    color: pal.LabelEx_valueText
                                    font.pixelSize: 12
                                }
                            }

                            // Type
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: "类型"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                }
                                Label {
                                    text: hasSelection
                                        ? (controller.currentFileInfo.typeCategoryName
                                           + " (" + controller.currentFileInfo.fileType + ")")
                                        : "-"
                                    color: pal.LabelEx_valueText
                                    font.pixelSize: 12
                                }
                            }

                            // Modified time
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: "修改时间"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                }
                                Label {
                                    text: hasSelection ? controller.currentFileInfo.modifiedTimeDisplay : "-"
                                    color: pal.LabelEx_valueText
                                    font.pixelSize: 12
                                }
                            }

                            // Created time
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: "创建时间"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                }
                                Label {
                                    text: hasSelection ? controller.currentFileInfo.createdTimeDisplay : "-"
                                    color: pal.LabelEx_valueText
                                    font.pixelSize: 12
                                }
                            }

                            // Path
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: "完整路径"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: hasSelection ? controller.currentFileInfo.filePath : "-"
                                    color: pal.LabelEx_pathText
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                    elide: Text.ElideRight
                                    maximumLineCount: 4
                                }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }
        }
    }
}
