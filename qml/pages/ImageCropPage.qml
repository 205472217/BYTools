import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Pane {
    id: root
    property var pal: themeManager.palette

    signal backRequested
    property var controller: null
    property var stackView: null
    property string pluginId: ""

    // ── Image rendering state ──────────────────────────────────────────
    property real srcW: 0
    property real srcH: 0
    property real dispW: 0
    property real dispH: 0

    // ── Crop rectangle state (in container coordinates) ────────────────
    property real cropX: 0
    property real cropY: 0
    property real cropW: 0
    property real cropH: 0
    property bool cropReady: false
    property bool cropMoving: false
    property bool updatingCropFromDrag: false

    // ── Tip display ────────────────────────────────────────────────────
    property string tipText: ""
    property bool showTip: false

    onShowTipChanged: {
        if (showTip) {
            tipDismissTimer.restart();
        }
    }

    Timer {
        id: tipDismissTimer
        interval: 1800
        onTriggered: root.showTip = false
    }

    Connections {
        target: controller
        function onCurrentFileCountChanged() {
            if (controller && controller.currentFileCount > 0)
                imageContainer.initImage();
        }
    }

    padding: 0
    background: Rectangle {
        id: bgRect
        color: pal.SurfaceEx_pageBg
    }

    FolderDialog {
        id: sourceFolderDialog
        title: "选择源文件夹"
        onAccepted: {
            if (controller) {
                var p = selectedFolder.toString();
                if (p.indexOf("file:///") === 0)
                    p = p.substring(8);
                else if (p.indexOf("file://") === 0)
                    p = p.substring(7);
                controller.rootPath = p;
            }
        }
    }

    FolderDialog {
        id: outputFolderDialog
        title: "选择输出目录"
        onAccepted: {
            if (controller) {
                var p = selectedFolder.toString();
                if (p.indexOf("file:///") === 0)
                    p = p.substring(8);
                else if (p.indexOf("file://") === 0)
                    p = p.substring(7);
                controller.outputDir = p;
            }
        }
    }

    ConfirmDialog {
        id: overflowWarnDialog
        property string detailText: ""
        dialogTitle: "裁剪尺寸超出图片"
        messageText: detailText
        onConfirmed: {
            if (controller) {
                var ok = controller.executeCrop(controller.cropX, controller.cropY, controller.cropW, controller.cropH);
                if (ok && controller.outputMode === 0)
                    root.initImage()
            }
        }
    }

    // ── Helper: calculate display size maintaining aspect ratio ─────────
    function calcDisplay() {
        if (!imageContainer.width || !imageContainer.height || !srcW || !srcH)
            return;
        var pad = 48;
        var cw = Math.max(imageContainer.width - pad * 2, 1);
        var ch = Math.max(imageContainer.height - pad * 2, 1);
        var scaleX = cw / srcW;
        var scaleY = ch / srcH;
        var s = Math.min(scaleX, scaleY);
        dispW = Math.round(srcW * s);
        dispH = Math.round(srcH * s);
    }

    // ── Initialize crop when image loads ───────────────────────────────
    function initImage() {
        if (!controller)
            return;
        var count = controller.getFileCount();
        if (count <= 0) {
            srcW = 0;
            srcH = 0;
            dispW = 0;
            dispH = 0;
            cropReady = false;
            imageCanvas.requestPaint();
            return;
        }
        var info = controller.getCurrentImageInfo();
        srcW = info.width || 0;
        srcH = info.height || 0;
        Qt.callLater(calcDisplay);
        Qt.callLater(initCrop);
    }

    function initCrop() {
        if (!controller || dispW <= 0 || dispH <= 0) {
            cropReady = false;
            return;
        }
        var result = controller.calcInitCropRect(Math.round(dispW), Math.round(dispH), Math.round(srcW), Math.round(srcH));
        if (result.cropReady) {
            cropX = result.x;
            cropY = result.y;
            cropW = result.w;
            cropH = result.h;
            cropReady = true;
            imageCanvas.requestPaint();
        } else {
            cropReady = false;
        }
    }

    // ── Sync crop state to controller ──────────────────────────────────
    function syncCropToController() {
        if (!controller || !cropReady || !srcW || !srcH)
            return;
        controller.syncCropToController(Math.round(cropX), Math.round(cropY), Math.round(cropW), Math.round(cropH), Math.round(dispW), Math.round(dispH), Math.round(srcW), Math.round(srcH));
    }

    // ── Navigation ─────────────────────────────────────────────────────
    function navNext() {
        if (!controller || controller.getFileCount() <= 0)
            return;
        var wasLast = (controller.currentIndex === controller.getFileCount() - 1);
        controller.navigateNext();
        initImage();
        if (wasLast) {
            tipText = "当前浏览第一张图片";
            showTip = true;
        }
    }

    function navPrev() {
        if (!controller || controller.getFileCount() <= 0)
            return;
        var wasFirst = (controller.currentIndex === 0);
        controller.navigatePrev();
        initImage();
        if (wasFirst) {
            tipText = "当前浏览最后一张图片";
            showTip = true;
        }
    }

    // ── Keyboard navigation ────────────────────────────────────────────
    Keys.onLeftPressed: navPrev()
    Keys.onRightPressed: navNext()

    // ── Main Layout ────────────────────────────────────────────────────
    ConfirmDialog {
        id: backConfirmDialog
        dialogTitle: "确认返回"
        messageText: "当前有图片裁剪任务正在处理中，返回首页将中断执行，是否继续？"
        onConfirmed: {
            if (controller) controller.reset()
            root.backRequested()
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
                iconSource: "qrc:/icons/global_back.svg"
                implicitHeight: 38
                tooltip: "返回"
                paletteGroup: "IconBtnEx"
                onClicked: {
                    if (controller && controller.isProcessing) {
                        backConfirmDialog.open();
                    } else {
                        root.backRequested();
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    id: pageTitle
                    text: "图片裁剪"
                    color: pal.LabelEx_titleText
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    id: pageSubtitle
                    text: "按比例或指定像素尺寸裁剪图片"
                    color: pal.LabelEx_subtitleText
                    font.pixelSize: 14
                }
            }
        }

        // Folder selection row
        Rectangle {
            id: folderRow
            Layout.fillWidth: true
            implicitHeight: 52
            radius: 10
            color: pal.SurfaceEx_cardBg
            border.color: pal.SurfaceEx_cardBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 0
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: 12

                Label {
                    id: folderLbl
                    text: "源文件夹"
                    color: pal.LabelEx_labelText
                    font.pixelSize: 13
                    font.bold: true
                }

                TextFieldEx {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    text: controller ? controller.rootPath : ""
                    readOnly: true
                    placeholderText: "点击浏览按钮选择图片文件夹"
                    clip: true
                    paletteGroup: "TextFieldEx"
                }

                CheckBoxEx {
                    id: recursiveCheck
                    implicitWidth: 110
                    text: "递归子文件夹"
                    paletteGroup: "CheckBoxEx"
                    checked: controller ? controller.recursive : false
                    onCheckedChanged: {
                        if (controller && controller.recursive !== checked)
                            controller.recursive = checked;
                    }
                }

                IconButton {
                    iconSource: "qrc:/icons/global_folder.svg"
                    tooltip: "选择源文件夹"
                    paletteGroup: "IconBtnEx"
                    onClicked: sourceFolderDialog.open()
                }

                IconButton {
                    id: loadImageBtn
                    text: "载入图片"
                    tooltip: "加载指定路径下的图片资源"
                    implicitWidth: 100
                    paletteGroup: "ImageCropPage_loadImageBtn"
                    onClicked: {
                        if (controller && controller.rootPath.length > 0)
                            controller.scanImages();
                    }
                }
            }
        }

        // Status bar + restore
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            spacing: 12
            visible: true

            Rectangle {
                id: statusBar
                Layout.fillWidth: true
                height: statusText.implicitHeight + 12
                radius: 6
                color: pal.SurfaceEx_statusBar

                Label {
                    id: statusText
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    text: controller && controller.statusMessage.length > 0
                        ? controller.statusMessage
                        : "就绪"
                    color: pal.LabelEx_statusText
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
            }

            IconButton {
                iconSource: "qrc:/icons/global_undo.svg"
                tooltip: "还原当前图片的裁剪"
                visible: controller ? controller.canRestoreCurrent : false
                paletteGroup: "IconBtnEx"
                onClicked: {
                    if (controller) {
                        controller.restoreCurrentFile()
                        root.initImage()
                    }
                }
            }
        }

        // Main content: Left (preview) + Right (controls)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            // ── Left Panel: Image Preview ──────────────────────────────
            Rectangle {
                id: imagePanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: pal.SurfaceEx_cardBg
                border.color: pal.SurfaceEx_cardBorder
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // Image container
                    Item {
                        id: imageContainer
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        property bool imageInitialized: false

                        function initImage() {
                            root.initImage();
                            imageInitialized = true;
                        }

                        onWidthChanged: {
                            if (imageInitialized && root.srcW > 0) {
                                root.calcDisplay();
                                root.initCrop();
                            }
                        }
                        onHeightChanged: {
                            if (imageInitialized && root.srcW > 0) {
                                root.calcDisplay();
                                root.initCrop();
                            }
                        }

                        Rectangle {
                            id: imgBg
                            anchors.fill: parent
                            radius: 6
                            color: root.dispW > 0 && root.dispH > 0 ? pal.ImageCropPage_imgBg_color : "transparent"

                            // Centered image area — sized to dispW/dispH but centered in parent
                            Item {
                                id: imageArea
                                width: Math.min(root.dispW, parent.width)
                                height: Math.min(root.dispH, parent.height)
                                anchors.centerIn: parent
                                visible: root.dispW > 0 && root.dispH > 0

                                Image {
                                    id: previewImage
                                    anchors.fill: parent
                                    source: controller && controller.currentFilePath.length > 0
                                        ? "file:///" + controller.currentFilePath + "?v=" + controller.imageVersion
                                        : ""
                                    fillMode: Image.PreserveAspectFit
                                    sourceSize: Qt.size(Math.max(root.dispW, 1), Math.max(root.dispH, 1))
                                    visible: root.dispW > 0 && root.dispH > 0
                                    cache: true
                                    asynchronous: true
                                    smooth: true
                                }

                                // ── Crop Overlay Canvas ──────────────────
                                Canvas {
                                    id: imageCanvas
                                    anchors.fill: parent
                                    visible: root.cropReady && root.dispW > 0

                                    onPaint: {
                                        var ctx = getContext("2d");
                                        ctx.clearRect(0, 0, width, height);
                                        if (!root.cropReady)
                                            return;

                                        // Dark semi-transparent overlay
                                        ctx.fillStyle = "rgba(0, 0, 0, 0.45)";
                                        ctx.fillRect(0, 0, width, height);

                                        // Clear the crop area
                                        ctx.clearRect(root.cropX, root.cropY, root.cropW, root.cropH);

                                        // White border for the crop area
                                        ctx.strokeStyle = "rgba(255, 255, 255, 0.95)";
                                        ctx.lineWidth = 1.5;
                                        ctx.strokeRect(root.cropX, root.cropY, root.cropW, root.cropH);

                                        // Grid lines (rule of thirds)
                                        ctx.strokeStyle = "rgba(255, 255, 255, 0.25)";
                                        ctx.lineWidth = 0.5;
                                        var gx1 = root.cropX + root.cropW / 3;
                                        var gx2 = root.cropX + root.cropW * 2 / 3;
                                        var gy1 = root.cropY + root.cropH / 3;
                                        var gy2 = root.cropY + root.cropH * 2 / 3;
                                        ctx.beginPath();
                                        ctx.moveTo(gx1, root.cropY);
                                        ctx.lineTo(gx1, root.cropY + root.cropH);
                                        ctx.moveTo(gx2, root.cropY);
                                        ctx.lineTo(gx2, root.cropY + root.cropH);
                                        ctx.moveTo(root.cropX, gy1);
                                        ctx.lineTo(root.cropX + root.cropW, gy1);
                                        ctx.moveTo(root.cropX, gy2);
                                        ctx.lineTo(root.cropX + root.cropW, gy2);
                                        ctx.stroke();
                                    }

                                    Connections {
                                        target: root
                                        function onCropXChanged() {
                                            imageCanvas.requestPaint();
                                        }
                                        function onCropYChanged() {
                                            imageCanvas.requestPaint();
                                        }
                                        function onCropWChanged() {
                                            imageCanvas.requestPaint();
                                        }
                                        function onCropHChanged() {
                                            imageCanvas.requestPaint();
                                        }
                                    }

                                    Connections {
                                        target: controller
                                        function onTargetWidthChanged() {
                                            if (controller && controller.cropMode === 1 && !root.updatingCropFromDrag)
                                                root.initCrop();
                                        }
                                        function onTargetHeightChanged() {
                                            if (controller && controller.cropMode === 1 && !root.updatingCropFromDrag)
                                                root.initCrop();
                                        }
                                    }
                                }

                                // ── Crop area mouse overlay (single MouseArea) ──
                                MouseArea {
                                    id: cropMouseArea
                                    anchors.fill: parent
                                    visible: root.cropReady
                                    preventStealing: true
                                    hoverEnabled: true

                                    // 0=none, 1=move, 2=TL, 3=TR, 4=BL, 5=BR
                                    property int dragMode: 0
                                    property real startMX: 0
                                    property real startMY: 0
                                    property real startCropX: 0
                                    property real startCropY: 0
                                    property real startCropW: 0
                                    property real startCropH: 0
                                    property real cornerHitSize: 14

                                    function cursorForMode(mode) {
                                        if (mode === 1)
                                            return Qt.SizeAllCursor;
                                        if (mode === 2)
                                            return Qt.SizeFDiagCursor;
                                        if (mode === 3)
                                            return Qt.SizeBDiagCursor;
                                        if (mode === 4)
                                            return Qt.SizeBDiagCursor;
                                        if (mode === 5)
                                            return Qt.SizeFDiagCursor;
                                        if (mode === 6 || mode === 7)
                                            return Qt.SizeVerCursor;
                                        if (mode === 8 || mode === 9)
                                            return Qt.SizeHorCursor;
                                        if (!containsMouse)
                                            return Qt.ArrowCursor;
                                        var h = controller ? controller.hitTest(mouseX, mouseY, root.cropX, root.cropY, root.cropW, root.cropH, cornerHitSize) : 0;
                                        if (h === 1)
                                            return Qt.SizeAllCursor;
                                        if (h === 2 || h === 5)
                                            return Qt.SizeFDiagCursor;
                                        if (h === 3 || h === 4)
                                            return Qt.SizeBDiagCursor;
                                        if (h === 6 || h === 7)
                                            return Qt.SizeVerCursor;
                                        if (h === 8 || h === 9)
                                            return Qt.SizeHorCursor;
                                        return Qt.ArrowCursor;
                                    }

                                    cursorShape: cursorForMode(dragMode > 0 ? dragMode : 0)

                                    onPressed: function (mouse) {
                                        var mode = controller ? controller.hitTest(mouse.x, mouse.y, root.cropX, root.cropY, root.cropW, root.cropH, cornerHitSize) : 0;
                                        if (mode === 0)
                                            return;
                                        dragMode = mode;
                                        startMX = mouse.x;
                                        startMY = mouse.y;
                                        startCropX = root.cropX;
                                        startCropY = root.cropY;
                                        startCropW = root.cropW;
                                        startCropH = root.cropH;
                                        root.cropMoving = true;
                                    }

                                    onPositionChanged: function (mouse) {
                                        if (dragMode === 0)
                                            return;
                                        var dx = mouse.x - startMX;
                                        var dy = mouse.y - startMY;
                                        var nx, ny, nw, nh, c, maxW, maxH;
                                        if (dragMode === 1) {
                                            // Move — clamp to image bounds
                                            nx = startCropX + dx;
                                            ny = startCropY + dy;
                                            root.cropX = Math.max(0, Math.min(nx, root.dispW - root.cropW));
                                            root.cropY = Math.max(0, Math.min(ny, root.dispH - root.cropH));
                                        } else if (dragMode === 2) {
                                            // TL — fixed corner is BR
                                            nw = startCropW - dx;
                                            nh = startCropH - dy;
                                            c = controller ? controller.constrainToRatio(nw, nh) : {w: nw, h: nh};
                                            nw = c.w;
                                            nh = c.h;
                                            maxW = startCropX + startCropW;
                                            maxH = startCropY + startCropH;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                nh = Math.round(nw / (controller ? controller.calcEffectiveRatio() : 1)) || nh;
                                            }
                                            if (nh > maxH) {
                                                nh = maxH;
                                                nw = Math.round(nh * (controller ? controller.calcEffectiveRatio() : 1)) || nw;
                                            }
                                            root.cropX = maxW - nw;
                                            root.cropY = maxH - nh;
                                            root.cropW = nw;
                                            root.cropH = nh;
                                        } else if (dragMode === 3) {
                                            // TR — fixed corner is BL
                                            nw = startCropW + dx;
                                            nh = startCropH - dy;
                                            c = controller ? controller.constrainToRatio(nw, nh) : {w: nw, h: nh};
                                            nw = c.w;
                                            nh = c.h;
                                            maxW = root.dispW - startCropX;
                                            maxH = startCropY + startCropH;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                nh = Math.round(nw / (controller ? controller.calcEffectiveRatio() : 1)) || nh;
                                            }
                                            if (nh > maxH) {
                                                nh = maxH;
                                                nw = Math.round(nh * (controller ? controller.calcEffectiveRatio() : 1)) || nw;
                                            }
                                            root.cropX = startCropX;
                                            root.cropY = maxH - nh;
                                            root.cropW = nw;
                                            root.cropH = nh;
                                        } else if (dragMode === 4) {
                                            // BL — fixed corner is TR
                                            nw = startCropW - dx;
                                            nh = startCropH + dy;
                                            c = controller ? controller.constrainToRatio(nw, nh) : {w: nw, h: nh};
                                            nw = c.w;
                                            nh = c.h;
                                            maxW = startCropX + startCropW;
                                            maxH = root.dispH - startCropY;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                nh = Math.round(nw / (controller ? controller.calcEffectiveRatio() : 1)) || nh;
                                            }
                                            if (nh > maxH) {
                                                nh = maxH;
                                                nw = Math.round(nh * (controller ? controller.calcEffectiveRatio() : 1)) || nw;
                                            }
                                            root.cropX = maxW - nw;
                                            root.cropY = startCropY;
                                            root.cropW = nw;
                                            root.cropH = nh;
                                        } else if (dragMode === 5) {
                                            // BR — fixed corner is TL
                                            nw = startCropW + dx;
                                            nh = startCropH + dy;
                                            c = controller ? controller.constrainToRatio(nw, nh) : {w: nw, h: nh};
                                            nw = c.w;
                                            nh = c.h;
                                            maxW = root.dispW - startCropX;
                                            maxH = root.dispH - startCropY;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                nh = Math.round(nw / (controller ? controller.calcEffectiveRatio() : 1)) || nh;
                                            }
                                            if (nh > maxH) {
                                                nh = maxH;
                                                nw = Math.round(nh * (controller ? controller.calcEffectiveRatio() : 1)) || nw;
                                            }
                                            root.cropX = startCropX;
                                            root.cropY = startCropY;
                                            root.cropW = nw;
                                            root.cropH = nh;
                                        } else if (dragMode === 6) {
                                            // Top edge — bottom fixed
                                            nh = startCropH - dy;
                                            nw = startCropW;
                                            if (controller) {
                                                c = controller.constrainToRatio(nw, nh);
                                                nw = c.w;
                                                nh = c.h;
                                            }
                                            maxH = startCropY + startCropH;
                                            if (nh > maxH) {
                                                nh = maxH;
                                                if (controller)
                                                    nw = Math.round(nh * controller.calcEffectiveRatio()) || nw;
                                            }
                                            if (nh < 20) nh = 20;
                                            root.cropY = maxH - nh;
                                            root.cropH = nh;
                                            root.cropW = nw;
                                        } else if (dragMode === 7) {
                                            // Bottom edge — top fixed
                                            nh = startCropH + dy;
                                            nw = startCropW;
                                            if (controller) {
                                                c = controller.constrainToRatio(nw, nh);
                                                nw = c.w;
                                                nh = c.h;
                                            }
                                            maxH = root.dispH - startCropY;
                                            if (nh > maxH) {
                                                nh = maxH;
                                                if (controller)
                                                    nw = Math.round(nh * controller.calcEffectiveRatio()) || nw;
                                            }
                                            if (nh < 20) nh = 20;
                                            root.cropY = startCropY;
                                            root.cropH = nh;
                                            root.cropW = nw;
                                        } else if (dragMode === 8) {
                                            // Left edge — right fixed
                                            nw = startCropW - dx;
                                            nh = startCropH;
                                            if (controller) {
                                                c = controller.constrainToRatio(nw, nh);
                                                nw = c.w;
                                                nh = c.h;
                                            }
                                            maxW = startCropX + startCropW;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                if (controller)
                                                    nh = Math.round(nw / controller.calcEffectiveRatio()) || nh;
                                            }
                                            if (nw < 20) nw = 20;
                                            root.cropX = maxW - nw;
                                            root.cropW = nw;
                                            root.cropH = nh;
                                        } else if (dragMode === 9) {
                                            // Right edge — left fixed
                                            nw = startCropW + dx;
                                            nh = startCropH;
                                            if (controller) {
                                                c = controller.constrainToRatio(nw, nh);
                                                nw = c.w;
                                                nh = c.h;
                                            }
                                            maxW = root.dispW - startCropX;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                if (controller)
                                                    nh = Math.round(nw / controller.calcEffectiveRatio()) || nh;
                                            }
                                            if (nw < 20) nw = 20;
                                            root.cropX = startCropX;
                                            root.cropW = nw;
                                            root.cropH = nh;
                                        }

                                        // ── Sync crop size to pixel values in pixel mode ──
                                        if (root.controller && root.controller.cropMode === 1 && dragMode !== 1) {
                                            root.updatingCropFromDrag = true;
                                            var sx = root.srcW / root.dispW;
                                            var sy = root.srcH / root.dispH;
                                            root.controller.targetWidth = Math.round(root.cropW * sx);
                                            root.controller.targetHeight = Math.round(root.cropH * sy);
                                            root.updatingCropFromDrag = false;
                                        }
                                    }

                                    onReleased: {
                                        dragMode = 0;
                                        root.cropMoving = false;
                                    }

                                    // Corner handles (visual indicators)
                                    Rectangle {
                                        id: handleTL
                                        x: root.cropX - 4
                                        y: root.cropY - 4
                                        width: 8
                                        height: 8
                                        radius: 2
                                        color: pal.ImageCropPage_handleTL_color
                                        border.color: pal.ImageCropPage_handleTL_borderColor
                                        border.width: 1.5
                                    }
                                    Rectangle {
                                        id: handleTR
                                        x: root.cropX + root.cropW - 4
                                        y: root.cropY - 4
                                        width: 8
                                        height: 8
                                        radius: 2
                                        color: pal.ImageCropPage_handleTR_color
                                        border.color: pal.ImageCropPage_handleTR_borderColor
                                        border.width: 1.5
                                    }
                                    Rectangle {
                                        id: handleBL
                                        x: root.cropX - 4
                                        y: root.cropY + root.cropH - 4
                                        width: 8
                                        height: 8
                                        radius: 2
                                        color: pal.ImageCropPage_handleBL_color
                                        border.color: pal.ImageCropPage_handleBL_borderColor
                                        border.width: 1.5
                                    }
                                    Rectangle {
                                        id: handleBR
                                        x: root.cropX + root.cropW - 4
                                        y: root.cropY + root.cropH - 4
                                        width: 8
                                        height: 8
                                        radius: 2
                                        color: pal.ImageCropPage_handleBR_color
                                        border.color: pal.ImageCropPage_handleBR_borderColor
                                        border.width: 1.5
                                    }

                                    // Edge midpoint handles
                                    Rectangle {
                                        id: handleTop
                                        x: root.cropX + root.cropW / 2 - 4
                                        y: root.cropY - 4
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: pal.ImageCropPage_handleTL_color
                                        border.color: pal.ImageCropPage_handleTL_borderColor
                                        border.width: 1.5
                                    }
                                    Rectangle {
                                        id: handleBottom
                                        x: root.cropX + root.cropW / 2 - 4
                                        y: root.cropY + root.cropH - 4
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: pal.ImageCropPage_handleBL_color
                                        border.color: pal.ImageCropPage_handleBL_borderColor
                                        border.width: 1.5
                                    }
                                    Rectangle {
                                        id: handleLeft
                                        x: root.cropX - 4
                                        y: root.cropY + root.cropH / 2 - 4
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: pal.ImageCropPage_handleTL_color
                                        border.color: pal.ImageCropPage_handleTL_borderColor
                                        border.width: 1.5
                                    }
                                    Rectangle {
                                        id: handleRight
                                        x: root.cropX + root.cropW - 4
                                        y: root.cropY + root.cropH / 2 - 4
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: pal.ImageCropPage_handleBR_color
                                        border.color: pal.ImageCropPage_handleBR_borderColor
                                        border.width: 1.5
                                    }
                                }
                            } // end imageArea

                            // Empty state
                            Column {
                                anchors.centerIn: parent
                                spacing: 8
                                visible: !root.cropReady

                                Label {
                                    id: emptyTitle
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请选择图片文件夹"
                                    color: pal.LabelEx_infoText
                                    font.pixelSize: 15
                                }
                                Label {
                                    id: emptySubtitle
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "支持 PNG、JPG、BMP、WebP、TIFF、GIF 格式"
                                    color: pal.LabelEx_infoText
                                    font.pixelSize: 12
                                }
                            }
                        }

                        // ── Previous button ────────────────────────
                        Rectangle {
                            id: prevBtn
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            width: 36
                            height: 60
                            radius: 10
                            color: prevBtnMouse.containsMouse ? pal.IconBtnEx_overlayBgHoverColor : pal.IconBtnEx_overlayBgColor
                            visible: controller && controller.currentFileCount > 0
                            opacity: prevBtnMouse.containsMouse ? 1.0 : 0.55

                            Behavior on opacity {
                                NumberAnimation {
                                    duration: 150
                                }
                            }

                            Label {
                                id: prevBtnIcon
                                anchors.centerIn: parent
                                text: "‹"
                                color: prevBtnMouse.containsMouse ? pal.IconBtnEx_overlayTextHoverColor : pal.IconBtnEx_overlayTextColor
                                font.pixelSize: 26
                                font.bold: true
                            }

                            MouseArea {
                                id: prevBtnMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.navPrev()
                            }
                        }

                        // ── Next button ────────────────────────────
                        Rectangle {
                            id: nextBtn
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 4
                            width: 36
                            height: 60
                            radius: 10
                            color: nextBtnMouse.containsMouse ? pal.IconBtnEx_overlayBgHoverColor : pal.IconBtnEx_overlayBgColor
                            visible: controller && controller.currentFileCount > 0
                            opacity: nextBtnMouse.containsMouse ? 1.0 : 0.55

                            Behavior on opacity {
                                NumberAnimation {
                                    duration: 150
                                }
                            }

                            Label {
                                id: nextBtnIcon
                                anchors.centerIn: parent
                                text: "›"
                                color: nextBtnMouse.containsMouse ? pal.IconBtnEx_overlayTextHoverColor : pal.IconBtnEx_overlayTextColor
                                font.pixelSize: 26
                                font.bold: true
                            }

                            MouseArea {
                                id: nextBtnMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.navNext()
                            }
                        }

                        // ── Page indicator ─────────────────────────
                        Rectangle {
                            id: pageIndicator
                            anchors.top: parent.top
                            anchors.topMargin: 8
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            width: pageLabel.implicitWidth + 16
                            height: 22
                            radius: 11
                            color: pal.SurfaceEx_pageIndicator
                            visible: controller && controller.currentFileCount > 0

                            Label {
                                id: pageLabel
                                anchors.centerIn: parent
                                text: {
                                    if (!controller)
                                        return "";
                                    var total = controller.getFileCount();
                                    var cur = controller.currentIndex + 1;
                                    return cur + "/" + total;
                                }
                                color: pal.LabelEx_valueText
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }

                        // ── Crop info label ───────────────────────
                        Rectangle {
                            id: cropInfoRect
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            width: cropInfoLabel.implicitWidth + 16
                            height: 22
                            radius: 6
                            color: pal.ImageCropPage_cropInfoRect_color
                            visible: root.cropReady

                            Label {
                                id: cropInfoLabel
                                anchors.centerIn: parent
                                text: {
                                    if (!root.cropReady)
                                        return "";
                                    var ratio = controller ? controller.calcEffectiveRatio() : 0;
                                    if (root.controller && root.controller.cropMode === 0 && ratio > 0) {
                                        return "裁剪比例 " + (ratio >= 1 ? ratio.toFixed(1) : ("1:" + (1 / ratio).toFixed(1)));
                                    } else if (root.controller && root.controller.cropMode === 1) {
                                        return "裁剪尺寸 " + root.controller.targetWidth + " × " + root.controller.targetHeight + " px";
                                    }
                                    return "";
                                }
                                color: pal.LabelEx_infoText
                                font.pixelSize: 11
                            }
                        }

                        // ── Image filename label ────────────────────
                        Rectangle {
                            id: fileNameRect
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: fileNameLabel.implicitWidth + 16
                            height: 22
                            radius: 6
                            color: pal.ImageCropPage_fileNameRect_color
                            visible: root.cropReady && controller && controller.currentFilePath.length > 0

                            Label {
                                id: fileNameLabel
                                anchors.centerIn: parent
                                text: controller ? controller.extractFileName(controller.currentFilePath) : ""
                                color: pal.LabelEx_pathText
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                                maximumLineCount: 1
                            }
                        }

                        // ── Image size label (bottom‑right) ─────────────
                        Rectangle {
                            id: imageSizeRect
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            width: imageSizeLabel.implicitWidth + 16
                            height: 22
                            radius: 6
                            color: pal.ImageCropPage_cropInfoRect_color
                            visible: root.srcW > 0 && root.srcH > 0

                            Label {
                                id: imageSizeLabel
                                anchors.centerIn: parent
                                text: root.srcW + " × " + root.srcH
                                color: pal.LabelEx_infoText
                                font.pixelSize: 11
                            }
                        }

                        // ── Auto-dismiss Tip (centered in image area) ──
                        Rectangle {
                            id: tipOverlay
                            anchors.centerIn: parent
                            width: tipLabel.implicitWidth + 32
                            height: 36
                            radius: 18
                            color: pal.SurfaceEx_overlay
                            z: 100
                            opacity: 0

                            Behavior on opacity {
                                NumberAnimation {
                                    duration: 200
                                }
                            }

                            Connections {
                                target: root
                                function onShowTipChanged() {
                                    tipOverlay.opacity = root.showTip ? 1 : 0;
                                }
                            }

                            Label {
                                id: tipLabel
                                anchors.centerIn: parent
                                text: root.tipText
                                color: pal.LabelEx_infoText
                                font.pixelSize: 13
                            }
                        }
                    }
                }
            }

            // ── Right Panel: Controls ──────────────────────────────────
            Rectangle {
                id: rightPanel
                Layout.preferredWidth: 310
                Layout.fillHeight: true
                radius: 10
                color: pal.SurfaceEx_cardBg
                border.color: pal.SurfaceEx_cardBorder
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 0

                    // ── Tab Titles ──────────────────────────────
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Rectangle {
                            id: ratioTab
                            Layout.fillWidth: true
                            height: 26
                            radius: 8
                            color: controller && controller.cropMode === 0 ? pal.ImageCropPage_ratioTab_color_active : pal.ImageCropPage_ratioTab_color_normal
                            border.color: controller && controller.cropMode === 0 ? pal.ImageCropPage_ratioTab_borderColor_active : pal.ImageCropPage_ratioTab_borderColor_normal
                            border.width: 1

                            Label {
                                id: ratioTabLbl
                                anchors.centerIn: parent
                                text: "按比例裁剪"
                                color: controller && controller.cropMode === 0 ? pal.ImageCropPage_ratioTabLbl_color_active : pal.ImageCropPage_ratioTabLbl_color_normal
                                font.pixelSize: 12
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (controller && controller.cropMode !== 0) {
                                        controller.cropMode = 0;
                                        root.initCrop();
                                    }
                                }
                            }
                        }

                        Rectangle {
                            id: pixelTab
                            Layout.fillWidth: true
                            height: 26
                            radius: 8
                            color: controller && controller.cropMode === 1 ? pal.ImageCropPage_pixelTab_color_active : pal.ImageCropPage_pixelTab_color_normal
                            border.color: controller && controller.cropMode === 1 ? pal.ImageCropPage_pixelTab_borderColor_active : pal.ImageCropPage_pixelTab_borderColor_normal
                            border.width: 1

                            Label {
                                id: pixelTabLbl
                                anchors.centerIn: parent
                                text: "按像素裁剪"
                                color: controller && controller.cropMode === 1 ? pal.ImageCropPage_pixelTabLbl_color_active : pal.ImageCropPage_pixelTabLbl_color_normal
                                font.pixelSize: 12
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (controller && controller.cropMode !== 1) {
                                        if (root.srcW > 0 && root.srcH > 0) {
                                            controller.targetWidth = Math.round(root.srcW);
                                            controller.targetHeight = Math.round(root.srcH);
                                        }
                                        controller.cropMode = 1;
                                        root.initCrop();
                                    }
                                }
                            }
                        }
                    }

                    // ── Tab Content: three equal sections ────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 0

                        // ── Section 1: Input Settings ─────────────
                        Rectangle {
                            id: inputSection
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.topMargin: 8
                            radius: 6
                            color: pal.SurfaceEx_cardBgAlt
                            border.color: pal.SurfaceEx_cardBorderLight
                            border.width: 1

                            ColumnLayout {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 8
                                spacing: 6

                                Label {
                                    id: inputSectionLbl
                                    text: "输入设置"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                // ── Ratio Mode Panel ──────────────
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    visible: controller ? controller.cropMode === 0 : false

                                    RowLayout {
                                        spacing: 4
                                        Repeater {
                                            model: ["1:1", "4:3", "16:9", "9:16"]

                                            Rectangle {
                                                id: ratioBtn
                                                property int ratioIdx: index
                                                property bool isSelected: controller && controller.usePresetRatio && controller.presetRatioIndex === ratioIdx
                                                Layout.preferredWidth: 42
                                                Layout.preferredHeight: 26
                                                radius: 5
                                                color: isSelected ? pal.ImageCropPage_ratioBtn_color_active : pal.ImageCropPage_ratioBtn_color_normal
                                                border.color: isSelected ? pal.ImageCropPage_ratioBtn_borderColor_active : pal.ImageCropPage_ratioBtn_borderColor_normal
                                                border.width: 1

                                                Label {
                                                    id: ratioBtnLbl
                                                    anchors.centerIn: parent
                                                    text: modelData
                                                    color: isSelected ? pal.ImageCropPage_ratioBtnLbl_color_active : pal.ImageCropPage_ratioBtnLbl_color_normal
                                                    font.pixelSize: 11
                                                    font.bold: true
}

                                                MouseArea {
                                                    anchors.fill: parent
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: {
                                                        if (controller) {
                                                            controller.usePresetRatio = true;
                                                            controller.presetRatioIndex = ratioIdx;
                                                            root.initCrop();
    }
}
                                                }
                                            }
                                        }

                                        // ── Free Ratio Button ─────────
                                        Rectangle {
                                            id: freeBtn
                                            Layout.preferredWidth: 42
                                            Layout.preferredHeight: 26
                                            radius: 5
                                            color: controller && controller.usePresetRatio && controller.presetRatioIndex === 4 ? pal.ImageCropPage_freeBtn_color_active : pal.ImageCropPage_freeBtn_color_normal
                                            border.color: controller && controller.usePresetRatio && controller.presetRatioIndex === 4 ? pal.ImageCropPage_freeBtn_borderColor_active : pal.ImageCropPage_freeBtn_borderColor_normal
                                            border.width: 1

                                            Label {
                                                id: freeBtnLbl
                                                anchors.centerIn: parent
                                                text: "自由"
                                                color: controller && controller.usePresetRatio && controller.presetRatioIndex === 4 ? pal.ImageCropPage_freeBtnLbl_color_active : pal.ImageCropPage_freeBtnLbl_color_normal
                                                font.pixelSize: 11
                                                font.bold: true
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    if (controller) {
                                                        controller.usePresetRatio = true;
                                                        controller.presetRatioIndex = 4;
                                                        root.initCrop();
                                                    }
                                                }
                                            }
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }
                                    }

                                    RowLayout {
                                        spacing: 8

                                        RadioButtonEx {
                                            id: customRatioRadio
                                            implicitWidth: 60
                                            text: "自定义"
                                            paletteGroup: "RadioButtonEx"
                                            checked: controller ? !controller.usePresetRatio : false
                                            font.pixelSize: 12
                                            onCheckedChanged: {
                                                if (controller && checked !== !controller.usePresetRatio) {
                                                    controller.usePresetRatio = !checked;
                                                    root.initCrop();
                                                }
                                            }
                                        }

                                        TextFieldEx {
                                            Layout.preferredWidth: 50
                                            placeholderText: "宽"
                                            enabled: controller ? !controller.usePresetRatio : false
                                            text: controller ? controller.customRatioW.toString() : "1"
                                            editType: 1
                                            minNumber: 1
                                            maxNumber: 100
                                            horizontalAlignment: TextInput.AlignHCenter
                                            onTextChanged: {
                                                if (!controller)
                                                    return;
                                                var v = parseInt(text);
                                                if (!isNaN(v)) {
                                                    controller.customRatioW = v;
                                                    if (controller.cropMode === 0 && !controller.usePresetRatio)
                                                        root.initCrop();
                                                }
                                            }
                                            onActiveFocusChanged: {
                                                if (!activeFocus && controller) {
                                                    var v = parseInt(text);
                                                    if (isNaN(v))
                                                        text = controller.customRatioW.toString();
                                                }
                                            }
                                            paletteGroup: "TextFieldEx"
                                        }

                                        Label {
                                            id: colonLbl
                                            text: ":"
                                            color: pal.LabelEx_infoText
                                            font.pixelSize: 14
                                            font.bold: true
                                        }

                                        TextFieldEx {
                                            Layout.preferredWidth: 50
                                            placeholderText: "高"
                                            enabled: controller ? !controller.usePresetRatio : false
                                            text: controller ? controller.customRatioH.toString() : "1"
                                            editType: 1
                                            minNumber: 1
                                            maxNumber: 100
                                            horizontalAlignment: TextInput.AlignHCenter
                                            onTextChanged: {
                                                if (!controller)
                                                    return;
                                                var v = parseInt(text);
                                                if (!isNaN(v)) {
                                                    controller.customRatioH = v;
                                                    if (controller.cropMode === 0 && !controller.usePresetRatio)
                                                        root.initCrop();
                                                }
                                            }
                                            onActiveFocusChanged: {
                                                if (!activeFocus && controller) {
                                                    var v = parseInt(text);
                                                    if (isNaN(v))
                                                        text = controller.customRatioH.toString();
                                                }
                                            }
                                            paletteGroup: "TextFieldEx"
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }
                                    }
                                }

                                // ── Pixel Mode Panel ──────────────
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    visible: controller ? controller.cropMode === 1 : false

                                    RowLayout {
                                        spacing: 8

                                        TextFieldEx {
                                            id: targetWidthField
                                            Layout.preferredWidth: 70
                                            placeholderText: "宽"
                                            text: controller ? controller.targetWidth.toString() : "800"
                                            editType: 1
                                            minNumber: 1
                                            maxNumber: 9999
                                            horizontalAlignment: TextInput.AlignHCenter
                                            onTextChanged: {
                                                if (controller) {
                                                    var v = parseInt(text);
                                                    if (!isNaN(v))
                                                        controller.targetWidth = v;
                                                }
                                            }
                                            onActiveFocusChanged: {
                                                if (!activeFocus && controller) {
                                                    var v = parseInt(text);
                                                    if (isNaN(v))
                                                        v = 800;
                                                    controller.targetWidth = v;
                                                    text = v.toString();
                                                }
                                            }
                                            paletteGroup: "TextFieldEx"
                                        }

                                        Label {
                                            id: multiplyLbl
                                            text: "x"
                                            color: pal.LabelEx_infoText
                                            font.pixelSize: 16
                                        }

                                        TextFieldEx {
                                            id: targetHeightField
                                            Layout.preferredWidth: 70
                                            placeholderText: "高"
                                            text: controller ? controller.targetHeight.toString() : "600"
                                            editType: 1
                                            minNumber: 1
                                            maxNumber: 9999
                                            horizontalAlignment: TextInput.AlignHCenter
                                            onTextChanged: {
                                                if (controller) {
                                                    var v = parseInt(text);
                                                    if (!isNaN(v))
                                                        controller.targetHeight = v;
                                                }
                                            }
                                            onActiveFocusChanged: {
                                                if (!activeFocus && controller) {
                                                    var v = parseInt(text);
                                                    if (isNaN(v))
                                                        v = 600;
                                                    controller.targetHeight = v;
                                                    text = v.toString();
                                                }
                                            }
                                            paletteGroup: "TextFieldEx"
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }
                        }

                        // ── Section 2: Separator ─────────────────
                        Rectangle {
                            id: sectionSep
                            Layout.fillWidth: true
                            Layout.preferredHeight: 16
                            color: "transparent"

                            Rectangle {
                                id: sectionSepLine
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: 1
                                color: pal.SurfaceEx_divider
                            }
                        }

                        // ── Section 3: Output Settings ───────────
                        Rectangle {
                            id: outputSection
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.bottomMargin: 8
                            radius: 6
                            color: pal.SurfaceEx_cardBgAlt
                            border.color: pal.SurfaceEx_cardBorderLight
                            border.width: 1

                            ColumnLayout {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 8
                                spacing: 6

                                Label {
                                    id: outputSectionLbl
                                    text: "输出设置"
                                    color: pal.LabelEx_labelText
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                RowLayout {
                                    spacing: 12

                                    RadioButtonEx {
                                        id: overwriteRadio
                                        implicitWidth: 120
                                        text: "覆盖源文件"
                                        paletteGroup: "RadioButtonEx"
                                        checked: controller ? controller.outputMode === 0 : true
                                        font.pixelSize: 12
                                        onCheckedChanged: {
                                            if (checked && controller && controller.outputMode !== 0)
                                                controller.outputMode = 0;
                                        }
                                    }

                                    RadioButtonEx {
                                        id: newDirRadio
                                        implicitWidth: 120
                                        text: "输出到新目录"
                                        paletteGroup: "RadioButtonEx"
                                        checked: controller ? controller.outputMode === 1 : false
                                        font.pixelSize: 12
                                        onCheckedChanged: {
                                            if (checked && controller && controller.outputMode !== 1)
                                                controller.outputMode = 1;
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 8
                                    visible: newDirRadio.checked

                                    TextFieldEx {
                                        id: outputDirField
                                        Layout.fillWidth: true
                                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                        text: controller ? controller.outputDir : ""
                                        placeholderText: controller ? (controller.rootPath ? controller.rootPath + "_cropped" : "输出到源目录_croppe") : ""
                                        readOnly: true
                                        clip: true
                                        onTextChanged: {
                                            if (controller)
                                                controller.outputDir = text;
                                        }

                                        ToolTip.visible: outputDirMouse.containsMouse && (text.length > 0 || placeholderText.length > 0)
                                        ToolTip.delay: 500
                                        ToolTip.text: text.length > 0 ? text : placeholderText

                                        MouseArea {
                                            id: outputDirMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            propagateComposedEvents: true
                                            onPressed: function (mouse) {
                                                mouse.accepted = false;
                                            }
                                        }
                                        paletteGroup: "TextFieldEx"
                                    }

                                    IconButton {
                                        iconSource: "qrc:/icons/global_folder.svg"
                                        tooltip: "选择输出目录"
                                        paletteGroup: "IconBtnEx"
                                        onClicked: outputFolderDialog.open()
                                    }
                                }
                            }
                        }
                    }

                    // ── Separator ───────────────────────────────
                    Rectangle {
                        id: bottomSep
                        Layout.fillWidth: true
                        height: 1
                        color: pal.SurfaceEx_dividerLight
                        Layout.topMargin: 12
                        Layout.bottomMargin: 12
                    }

                    // ── Action Buttons (fixed at bottom) ────────
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        IconButton {
                            id: execBtn
                            implicitWidth: 150
                            Layout.fillWidth: true
                            text: "开始处理"
                            iconSource: "qrc:/icons/global_start.svg"
                            tooltip: "开始裁剪图片"
                            paletteGroup: "ImageCropPage_execBtn"
                            onClicked: {
                                if (!controller || !root.cropReady)
                                    return;
                                root.syncCropToController();

                                // Check if target dimensions exceed image (pixel mode)
                                if (controller.cropMode === 1 && (root.srcW < controller.targetWidth || root.srcH < controller.targetHeight)) {
                                    overflowWarnDialog.detailText = "目标尺寸 " + controller.targetWidth + " × " + controller.targetHeight + " 超出图片实际尺寸 " + root.srcW + " × " + root.srcH + "。\n\n实际裁剪尺寸将以图片可用区域为准，是否继续？";
                                    overflowWarnDialog.open();
                                    return;
                                }
                                var ok = controller.executeCrop(controller.cropX, controller.cropY, controller.cropW, controller.cropH);
                                if (ok && controller.outputMode === 0) {
                                    root.initImage();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}


