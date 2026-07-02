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

    property bool hasFiles: controller && controller.fileCount > 0
    property bool hasSelection: controller && controller.currentFilePath.length > 0
    property string _lastLogLine: ""
    property int _activeViewWay: 0  // 上次扫描所用的浏览方式，默认目录模式
    property var _typeNames: ["视频", "音频", "图片", "全部"]
    property int _contextMenuIndex: -1
    readonly property var _activeView: controller && controller.viewMode === 1 ? fileGridView : fileListView

    MessageDialog {
        id: confirmDeleteDialog
        title: "确认删除"
        text: "确定要永久删除该文件吗？\n此操作不可恢复！"
        buttons: MessageDialog.Yes | MessageDialog.No
        onAccepted: {
            if (!controller || controller.currentModelIndex < 0)
                return;
            if (controller.currentModelIndex === root._contextMenuIndex) {
                if (videoPreviewLoader.item && videoPreviewLoader.active) {
                    videoPreviewLoader.item.stop();
                    videoPreviewLoader.item.source = "";
                }
                if (imagePreviewLoader.item && imagePreviewLoader.active)
                    imagePreviewLoader.item.source = "";
                controller.deleteFile(controller.currentModelIndex);
            }
        }
    }

    // ── mpv 检测（启动时已解压，此处仅检查文件是否存在） ──
    property bool _mpvAvailable: {
        if (!controller)
            return false;
        var dir = pluginManager.pluginDirectory("file-view");
        return dir.length > 0 && pluginManager.fileExists(dir + "/mpv/mpv.exe");
    }
    property string _mpvExePath: _mpvAvailable ? pluginManager.pluginDirectory("file-view") + "/mpv/mpv.exe" : ""

    function safeInfo(key, fallback) {
        if (!controller || !hasSelection)
            return fallback || "-";
        var v = controller.currentFileInfo[key];
        return v !== undefined ? v : (fallback || "-");
    }

    function fmtSize(bytes) {
        if (bytes < 1024)
            return bytes + " B";
        if (bytes < 1048576)
            return (bytes / 1024).toFixed(1) + " KB";
        if (bytes < 1073741824)
            return (bytes / 1048576).toFixed(1) + " MB";
        return (bytes / 1073741824).toFixed(2) + " GB";
    }

    function _hideNativeOverlay() {
        var p = videoPreviewLoader.item;
        if (p && p.setNativeOverlayVisible)
            p.setNativeOverlayVisible(false);
    }
    function _showNativeOverlay() {
        var p = videoPreviewLoader.item;
        if (p && p.setNativeOverlayVisible)
            p.setNativeOverlayVisible(true);
    }

    function selectedFileUrl(url) {
        var p = url.toString();
        if (p.indexOf("file:///") === 0)
            return p.substring(8);
        if (p.indexOf("file://") === 0)
            return p.substring(7);
        return p;
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
            if (controller)
                controller.sourceFolder = selectedFileUrl(selectedFolder);
        }
    }

    // ── Controller signal connections ──
    Connections {
        target: controller
        function onLogMessage(message) {
            if (message.length === 0)
                return;
            root._lastLogLine = message;
        }
        function onScanFinished() {
            if (controller && controller.fileCount > 0)
                root._lastLogLine = "扫描完成，共 " + controller.fileCount + " 个文件";
        }
        function onCurrentModelIndexChanged() {
            if (controller) {
                fileListView.currentIndex = controller.currentModelIndex;
                fileGridView.currentIndex = controller.currentModelIndex;
            }
        }
    }

    function navPrevFile() {
        if (!controller)
            return;
        var idx = controller.prevFileInCategory(controller.currentModelIndex);
        if (idx >= 0)
            controller.selectFile(idx);
    }

    function navNextFile() {
        if (!controller)
            return;
        var idx = controller.nextFileInCategory(controller.currentModelIndex);
        if (idx >= 0)
            controller.selectFile(idx);
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
                    root._hideNativeOverlay();
                    root.backRequested();
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

                // Row 1: Source folder
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

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择源文件夹"
                        paletteGroup: "IconBtnEx"
                        onClicked: folderDialog.open()
                    }
                }

                // Row 2: File type radio
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: filterTypeLabel
                        text: "筛选类型"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    Repeater {
                        model: root._typeNames

                        RadioButtonEx {
                            required property int index
                            required property string modelData

                            text: modelData
                            paletteGroup: "RadioButtonEx"
                            checked: controller ? controller.fileType === index : index === 0
                            font.pixelSize: 13
                            onCheckedChanged: {
                                if (checked && controller && controller.fileType !== index)
                                    controller.fileType = index;
                            }
                        }
                    }
                }

                // Row 3: Start scan button
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        id: viewTypeLabel
                        text: "浏览方式"
                        color: pal.LabelEx_labelText
                        font.pixelSize: 12
                        font.bold: true
                        Layout.preferredWidth: 72
                    }

                    RadioButtonEx {
                        id: dirModeRadio
                        text: "目录模式"
                        paletteGroup: "RadioButtonEx"
                        checked: controller.viewWay === 0
                        font.pixelSize: 13
                        onCheckedChanged: {
                            if (checked && controller && controller.viewWay !== 0) {
                                controller.viewWay = 0
                            }
                        }
                    }

                    RadioButtonEx {
                        id: fileModeRadio
                        text: "文件模式"
                        paletteGroup: "RadioButtonEx"
                        checked: controller.viewWay === 1
                        font.pixelSize: 13
                        onCheckedChanged: {
                            if (checked && controller && controller.viewWay !== 1) {
                                controller.viewWay = 1
                            }
                        }
                    }

                    CheckBoxEx {
                        id: recursiveCheck
                        implicitWidth: 110
                        text: "递归源文件夹"
                        paletteGroup: "CheckBoxEx"
                        font.pixelSize: 12
                        checked: controller ? controller.recursive : false
                        visible: controller ? controller.viewWay === 1 : false
                        onCheckedChanged: {
                            if (controller && controller.recursive !== checked)
                                controller.recursive = checked;
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    IconButton {
                        id: startScanBtn
                        implicitWidth: 150
                        text: "开始浏览"
                        tooltip: "扫描所选文件夹中的文件"
                        paletteGroup: "FileViewPage_scanBtn"
                        enabled: controller && controller.sourceFolder.length > 0
                        onClicked: {
                            if (!controller)
                                return;
                            root._activeViewWay = controller.viewWay;
                            controller.startScan();
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

                    // Top bar: 文件模式=标题, 目录模式=路径导航
                    Rectangle {
                        id: topBar
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        radius: 6
                        color: pal.SurfaceEx_headerRowBg
                        border.color: pal.SurfaceEx_cardBorderLight
                        border.width: 1

                        // 目录模式：返回 + 路径 + 切换视图
                        RowLayout {
                            visible: _activeViewWay === 0
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 4
                            spacing: 6

                            IconButton {
                                implicitWidth: 24
                                implicitHeight: 24
                                iconSource: "qrc:/icons/arrow-left.svg"
                                tooltip: "返回上级目录"
                                paletteGroup: "IconBtnEx"
                                enabled: controller && controller.canNavigateUp
                                onClicked: controller.navigateUp()
                            }

                            TextFieldEx {
                                Layout.fillWidth: true
                                implicitHeight: 24
                                text: controller ? controller.currentPath : ""
                                readOnly: true
                                font.pixelSize: 11
                                paletteGroup: "TextFieldEx"
                            }

                            ViewToggleBtn {
                                iconType: controller && controller.viewMode === 0 ? "grid" : "list"
                                tip: controller && controller.viewMode === 0 ? "网格" : "列表"
                                paletteGroup: "ViewToggleBtn"
                                onClicked: {
                                    if (controller)
                                        controller.viewMode = controller.viewMode === 0 ? 1 : 0
                                }
                            }
                        }

                        // 文件模式：标题 + 切换视图
                        RowLayout {
                            visible: _activeViewWay === 1
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 4
                            spacing: 6

                            Label {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.leftMargin: 12
                                text: "文件模式"
                                color: pal.LabelEx_labelText
                                font.pixelSize: 13
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            ViewToggleBtn {
                                iconType: controller && controller.viewMode === 0 ? "grid" : "list"
                                tip: controller && controller.viewMode === 0 ? "网格" : "列表"
                                paletteGroup: "ViewToggleBtn"
                                onClicked: {
                                    if (controller)
                                        controller.viewMode = controller.viewMode === 0 ? 1 : 0
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

                    // List body + Grid body
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        // ── List View ──
                        ListView {
                            id: fileListView
                            anchors.fill: parent
                            model: controller ? controller.fileListModel : null
                            visible: controller && controller.viewMode === 0 && hasFiles
                            currentIndex: controller ? controller.currentModelIndex : -1
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
                                required property string filePath
                                required property string fileSizeDisplay
                                required property string modifiedTimeDisplay
                                required property string fileType
                                required property int typeCategory
                                required property bool isDir

                                property bool rowHovered: false

                                color: {
                                    if (ListView.isCurrentItem)
                                        return pal.SurfaceEx_rowSelectedBg;
                                    return index % 2 === 0 ? pal.SurfaceEx_rowEvenBg : pal.SurfaceEx_rowOddBg;
                                }

                                RowLayout {
                                    id: rowContent
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
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
                                                color: pal.LabelEx_valueText
                                                font.pixelSize: 12
                                                elide: Text.ElideRight
                                            }

                                            Rectangle {
                                                id: typeTag
                                                width: typeTagText.implicitWidth + 10
                                                height: 18
                                                radius: 3
                                                color: {
                                                    switch (fileDelegate.typeCategory) {
                                                    case 0:
                                                        return pal.LabelEx_Video_BgRect;
                                                    case 1:
                                                        return pal.LabelEx_Audio_BgRect;
                                                    case 2:
                                                        return pal.LabelEx_Image_BgRect;
                                                    default:
                                                        return pal.LabelEx_Other_BgRect;
                                                    }
                                                }

                                                Label {
                                                    id: typeTagText
                                                    anchors.centerIn: parent
                                                    text: fileDelegate.fileType
                                                    color: {
                                                        switch (fileDelegate.typeCategory) {
                                                        case 0:
                                                            return pal.LabelEx_Video_Text;
                                                        case 1:
                                                            return pal.LabelEx_Audio_Text;
                                                        case 2:
                                                            return pal.LabelEx_Image_Text;
                                                        default:
                                                            return pal.LabelEx_Other_Text;
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

                                            Item {
                                                Layout.preferredWidth: 28
                                            }

                                            Label {
                                                text: fileDelegate.fileSizeDisplay
                                                color: pal.LabelEx_infoText
                                                font.pixelSize: 10
                                                elide: Text.ElideRight
                                            }

                                            Item {
                                                Layout.fillWidth: true
                                            }

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
                                    z: 0
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    onEntered: fileDelegate.rowHovered = true
                                    onExited: fileDelegate.rowHovered = false
                                    onClicked: function (mouse) {
                                        if (mouse.button === Qt.RightButton) {
                                            root._contextMenuIndex = fileDelegate.index;
                                            contentMenu.popup();
                                        } else if (fileDelegate.isDir) {
                                            if (controller)
                                                controller.navigateToDir(fileDelegate.filePath);
                                        } else {
                                            if (controller)
                                                controller.selectFile(fileDelegate.index);
                                        }
                                    }
                                }
                            }
                        }

                        // ── Grid View ──
                        GridView {
                            id: fileGridView
                            anchors.fill: parent
                            anchors.margins: 4
                            model: controller ? controller.fileListModel : null
                            visible: controller && controller.viewMode === 1 && hasFiles
                            cellWidth: 150
                            cellHeight: 230
                            currentIndex: controller ? controller.currentModelIndex : -1
                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                            }
                            highlightMoveDuration: 0

                            readonly property real _gridThumbInset: 20 // delegate margin(8) + layout margin(12)

                            delegate: Item {
                                id: gridDelegateRoot
                                width: fileGridView.cellWidth
                                height: fileGridView.cellHeight
                                required property int index
                                required property string fileName
                                required property string filePath
                                required property string fileType
                                required property int typeCategory
                                required property bool isDir
                                required property string thumbnailPath

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    radius: 8
                                    color: fileGridView.currentIndex === index
                                        ? pal.SurfaceEx_rowSelectedBg : "transparent"

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 4

                                        Item {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: (fileGridView.cellWidth - fileGridView._gridThumbInset) * 4 / 3
                                            Layout.alignment: Qt.AlignHCenter

                                            Rectangle {
                                                anchors.fill: parent
                                                radius: 6
                                                color: pal.SurfaceEx_cardBgAlt
                                                clip: true

                                                Image {
                                                    anchors.fill: parent
                                                    asynchronous: true
                                                    source: {
                                                        if (thumbnailPath.length > 0)
                                                            return "file:///" + thumbnailPath.replace(/\\/g, "/");
                                                        return isDir
                                                            ? "qrc:/icons/thumb_folder.svg"
                                                            : "qrc:/icons/thumb_file.svg";
                                                    }
                                                    fillMode: isDir ? Image.PreserveAspectFit : Image.PreserveAspectCrop
                                                    sourceSize.width: 256
                                                    sourceSize.height: 256
                                                }
                                            }
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: fileName
                                            color: pal.LabelEx_valueText
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                            horizontalAlignment: Text.AlignHCenter
                                            maximumLineCount: 2
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    onClicked: function (mouse) {
                                        if (mouse.button === Qt.RightButton) {
                                            root._contextMenuIndex = index;
                                            contentMenu.popup();
                                        } else if (isDir) {
                                            controller.navigateToDir(filePath);
                                        } else {
                                            controller.selectFile(index);
                                        }
                                    }
                                }
                            }
                        }

                        Menu {
                            id: contentMenu

                            MenuItem {
                                text: "定位到当前文件"
                                icon.source: "qrc:/icons/to-current.svg"
                                onTriggered: {
                                    var idx = root._activeView.currentIndex;
                                    if (idx >= 0)
                                        root._activeView.positionViewAtIndex(idx, ListView.Contain);
                                }
                            }
                            MenuItem {
                                text: "定位到顶部"
                                icon.source: "qrc:/icons/to-top.svg"
                                onTriggered: root._activeView.positionViewAtBeginning()
                            }
                            MenuItem {
                                text: "定位到底部"
                                icon.source: "qrc:/icons/to-bottom.svg"
                                onTriggered: root._activeView.positionViewAtEnd()
                            }

                            MenuSeparator {}

                            Menu {
                                title: "排序方式"
                                icon.source: "qrc:/icons/empty.svg"
                                icon.width: 16
                                icon.height: 16

                                MenuItem {
                                    text: "名称"
                                    checkable: true
                                    checked: controller ? controller.sortField === 0 : false
                                    onTriggered: if (controller) controller.sortField = 0
                                }
                                MenuItem {
                                    text: "大小"
                                    checkable: true
                                    checked: controller ? controller.sortField === 3 : false
                                    onTriggered: if (controller) controller.sortField = 3
                                }
                                MenuItem {
                                    text: "日期"
                                    checkable: true
                                    checked: controller ? (controller.sortField === 1 || controller.sortField === 2) : false
                                    onTriggered: if (controller) controller.sortField = 1
                                }
                                MenuItem {
                                    text: "类型"
                                    checkable: true
                                    checked: controller ? controller.sortField === 4 : false
                                    onTriggered: if (controller) controller.sortField = 4
                                }

                                MenuSeparator {}

                                MenuItem {
                                    text: "递增"
                                    checkable: true
                                    checked: controller ? controller.sortAscending : true
                                    onTriggered: if (controller) controller.sortAscending = true
                                }
                                MenuItem {
                                    text: "递减"
                                    checkable: true
                                    checked: controller ? !controller.sortAscending : false
                                    onTriggered: if (controller) controller.sortAscending = false
                                }
                            }

                            MenuSeparator {}

                            MenuItem {
                                text: "删除"
                                icon.source: "qrc:/icons/trash.svg"
                                enabled: controller && controller.currentModelIndex >= 0 && root._contextMenuIndex === controller.currentModelIndex
                                onTriggered: confirmDeleteDialog.open()
                            }
                        }

                        // Empty state
                        Label {
                            id: emptyListLabel
                            anchors.centerIn: parent
                            text: {
                                if (controller.viewWay === 0)
                                    return "暂无内容，请选择源文件夹后点击「开始浏览」";
                                return "暂无文件，请选择源文件夹后点击「开始浏览」";
                            }
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
                            active: hasSelection && controller && (controller.currentFileInfo.typeCategory === 0 || controller.currentFileInfo.typeCategory === 1)
                            source: root._mpvAvailable ? "../components/MpvViewer.qml" : "../components/MediaViewer.qml"

                            onLoaded: {
                                if (controller) {
                                    if (root._mpvAvailable)
                                        item.mpvPath = root._mpvExePath;
                                    item.controlsPaletteGroup = "VideoPlayerControls";
                                    item.source = "file:///" + controller.currentFilePath;
                                    if (controller) {
                                        item.volume = controller.volume;
                                        item.muted = controller.muted;
                                        item.seekStepMs = controller.seekStepMs;
                                        if (item.volume < 1)
                                            item.muted = true;
                                    }
                                    item.play();
                                    item.previousRequested.connect(function () {
                                        if (!controller || controller.fileCount === 0)
                                            return;
                                        var idx = controller.prevFileInCategory(controller.currentModelIndex);
                                        if (idx >= 0)
                                            controller.selectFile(idx);
                                    });
                                    item.nextRequested.connect(function () {
                                        if (!controller || controller.fileCount === 0)
                                            return;
                                        var idx = controller.nextFileInCategory(controller.currentModelIndex);
                                        if (idx >= 0)
                                            controller.selectFile(idx);
                                    });
                                    item.deleteRequested.connect(function () {
                                        if (!controller || controller.fileCount === 0 || controller.currentModelIndex < 0)
                                            return;
                                        root._contextMenuIndex = controller.currentModelIndex;
                                        confirmDeleteDialog.open();
                                    });
                                }
                            }

                            Connections {
                                target: controller
                                function onCurrentFilePathChanged() {
                                    var p = videoPreviewLoader.item;
                                    if (p && controller) {
                                        var newSrc = "file:///" + controller.currentFilePath;
                                        if (p.source !== newSrc) {
                                            p.source = newSrc;
                                            p.play();
                                        }
                                    }
                                }
                            }

                            Connections {
                                target: videoPreviewLoader.item
                                function onVolumeChanged() {
                                    if (controller && videoPreviewLoader.item)
                                        controller.volume = videoPreviewLoader.item.volume;
                                }
                                function onMutedChanged() {
                                    if (controller && videoPreviewLoader.item)
                                        controller.muted = videoPreviewLoader.item.muted;
                                }
                                function onSeekStepMsChanged() {
                                    if (controller && videoPreviewLoader.item)
                                        controller.seekStepMs = videoPreviewLoader.item.seekStepMs;
                                }
                            }
                        }

                        // ── Image preview (Loader to avoid decode errors on non‑image files) ──
                        Loader {
                            id: imagePreviewLoader
                            anchors.fill: parent
                            active: hasSelection && controller && controller.currentFileInfo.typeCategory === 2
                            source: "../components/ImageViewer.qml"

                            onLoaded: {
                                if (!controller)
                                    return;
                                item.source = controller.currentFilePath;
                                item.hasPrevious = controller.prevFileInCategory(controller.currentModelIndex) >= 0;
                                item.hasNext = controller.nextFileInCategory(controller.currentModelIndex) >= 0;
                                item.previousRequested.connect(function () {
                                    navPrevFile();
                                });
                                item.nextRequested.connect(function () {
                                    navNextFile();
                                });
                                item.deleteRequested.connect(function () {
                                    if (!controller || controller.fileCount === 0 || controller.currentModelIndex < 0)
                                        return;
                                    root._contextMenuIndex = controller.currentModelIndex;
                                    confirmDeleteDialog.open();
                                });
                            }

                            Connections {
                                target: controller
                                function onCurrentFilePathChanged() {
                                    var p = imagePreviewLoader.item;
                                    if (p && controller)
                                        p.source = controller.currentFilePath;
                                }
                                function onCurrentModelIndexChanged() {
                                    var p = imagePreviewLoader.item;
                                    if (p && controller) {
                                        p.hasPrevious = controller.prevFileInCategory(controller.currentModelIndex) >= 0;
                                        p.hasNext = controller.nextFileInCategory(controller.currentModelIndex) >= 0;
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
                                    text: hasSelection ? (controller.currentFileInfo.typeCategoryName + " (" + controller.currentFileInfo.fileType + ")") : "-"
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

                            // Image resolution
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                visible: hasSelection && controller.currentFileInfo.typeCategory === 2

                                Label {
                                    text: "分辨率"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: controller ? controller.currentFileInfo.imageResolution : "-"
                                    color: pal.LabelEx_valueText
                                    font.pixelSize: 12
                                }
                            }

                            // Image bit depth
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                visible: hasSelection && controller.currentFileInfo.typeCategory === 2

                                Label {
                                    text: "位深度"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: controller ? controller.currentFileInfo.imageBitDepth : "-"
                                    color: pal.LabelEx_valueText
                                    font.pixelSize: 12
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
    }
}
