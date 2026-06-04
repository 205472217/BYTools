import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Pane {
    id: root

    signal backRequested
    property var controller: null

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

    Component.onDestruction: {
        if (controller && typeof controller.reset === 'function') {
            controller.reset();
        }
    }

    padding: 0
    background: Rectangle {
        color: "#f4f6f9"
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
                controller.scanImages();
                imageContainer.initImage();
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

    // ── Recursive toggle confirmation dialog ─────────────────────────────
    Dialog {
        id: recursiveConfirmDialog
        title: "确认切换"
        modal: true
        anchors.centerIn: parent
        width: 360
        standardButtons: Dialog.Ok | Dialog.Cancel

        property bool pendingRecursiveValue: false

        contentItem: Label {
            text: "切换「递归子文件夹」将重新扫描图片列表。\n\n当前已浏览的进度将丢失，是否继续？"
            wrapMode: Text.WordWrap
            color: "#334155"
            font.pixelSize: 13
            lineHeight: 1.4
            padding: 16
        }

        onAccepted: {
            if (controller) {
                controller.recursive = pendingRecursiveValue;
                if (controller.rootPath.length > 0) {
                    controller.scanImages();
                    imageContainer.initImage();
                }
            }
        }

        onRejected: {
            // Revert checkbox to match controller state
            recursiveCheck.checked = controller ? controller.recursive : false;
        }
    }

    // ── Crop overflow warning dialog ─────────────────────────────────────
    Dialog {
        id: overflowWarnDialog
        title: "裁剪尺寸超出图片"
        modal: true
        anchors.centerIn: parent
        width: 400
        standardButtons: Dialog.Ok | Dialog.Cancel

        property string detailText: ""

        contentItem: Label {
            text: overflowWarnDialog.detailText
            wrapMode: Text.WordWrap
            color: "#334155"
            font.pixelSize: 13
            lineHeight: 1.4
            padding: 16
        }

        onAccepted: {
            if (controller) {
                controller.executeCrop(controller.cropX, controller.cropY, controller.cropW, controller.cropH);
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

    // ── Helper: extract filename from path ──────────────────────────────
    function fileNameFromPath(filePath) {
        if (!filePath || filePath.length === 0)
            return "";
        var p = filePath.replace(/\\/g, "/");
        var idx = p.lastIndexOf("/");
        if (idx >= 0)
            p = p.substring(idx + 1);
        return p;
    }

    // ── Helper: current effective aspect ratio ─────────────────────────
    // Returns >0 for locked ratio, 0 for free ratio or pixel mode
    function effectiveRatio() {
        if (!controller)
            return 1;
        if (controller.cropMode === 0) {
            if (controller.usePresetRatio) {
                // Index 5 = "自由" (free ratio)
                if (controller.presetRatioIndex === 4)
                    return 0;
                var ratios = [[1, 1], [4, 3], [16, 9], [9, 16]];
                var idx = controller.presetRatioIndex;
                if (idx >= 0 && idx < ratios.length) {
                    return ratios[idx][0] / ratios[idx][1];
                }
                return 1;
            } else {
                var cw = controller.customRatioW;
                var ch = controller.customRatioH;
                return (cw > 0 && ch > 0) ? cw / ch : 1;
            }
        }
        return 0; // pixel mode
    }

    function isFreeRatio() {
        return effectiveRatio() === 0;
    }

    // ── Constrain w/h to current ratio ─────────────────────────────────
    // rawW/rawH = distance from fixed corner to mouse
    function constrainToRatio(rawW, rawH) {
        var ratio = effectiveRatio();
        if (ratio <= 0)
            return {
                w: Math.max(20, rawW),
                h: Math.max(20, rawH)
            };

        // Project (rawW, rawH) onto ratio diagonal direction (ratio, 1)
        var t = (rawW * ratio + rawH) / (ratio * ratio + 1);
        var nw = Math.round(t * ratio);
        var nh = Math.round(t);
        if (nw < 20) {
            nw = 20;
            nh = Math.round(nw / ratio);
        }
        if (nh < 20) {
            nh = 20;
            nw = Math.round(nh * ratio);
        }
        return {
            w: nw,
            h: nh
        };
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
        if (dispW <= 0 || dispH <= 0) {
            cropReady = false;
            return;
        }
        if (controller && controller.cropMode === 1) {
            // Pixel mode: map target pixels to screen
            var scaleX = dispW / srcW;
            var scaleY = dispH / srcH;
            var pw = controller.targetWidth * scaleX;
            var ph = controller.targetHeight * scaleY;
            pw = Math.min(pw, dispW);
            ph = Math.min(ph, dispH);
            cropW = Math.round(pw);
            cropH = Math.round(ph);
        } else {
            var r = effectiveRatio();
            if (r > 0) {
                // Ratio mode: fit largest rectangle with target ratio
                var fitW = dispH * r;
                if (fitW <= dispW) {
                    cropW = Math.round(fitW);
                    cropH = Math.round(dispH);
                } else {
                    cropW = Math.round(dispW);
                    cropH = Math.round(dispW / r);
                }
            } else {
                // Free ratio: use full display area
                cropW = Math.round(dispW);
                cropH = Math.round(dispH);
            }
        }
        cropX = Math.round((dispW - cropW) / 2);
        cropY = Math.round((dispH - cropH) / 2);
        cropReady = true;
        imageCanvas.requestPaint();
    }

    // ── Clamp crop area to display bounds ──────────────────────────────
    function clampCrop() {
        if (!cropReady)
            return;
        cropW = Math.max(20, Math.min(cropW, dispW));
        cropH = Math.max(20, Math.min(cropH, dispH));
        cropX = Math.max(0, Math.min(cropX, dispW - cropW));
        cropY = Math.max(0, Math.min(cropY, dispH - cropH));
    }

    // ── Sync crop state to controller ──────────────────────────────────
    function syncCropToController() {
        if (!controller || !cropReady || !srcW || !srcH)
            return;
        var scaleX = srcW / dispW;
        var scaleY = srcH / dispH;
        controller.cropX = Math.round(cropX * scaleX);
        controller.cropY = Math.round(cropY * scaleY);
        if (controller.cropMode === 1) {
            // Pixel mode: use exact target dimensions, no rounding error
            controller.cropW = controller.targetWidth;
            controller.cropH = controller.targetHeight;
        } else {
            controller.cropW = Math.round(cropW * scaleX);
            controller.cropH = Math.round(cropH * scaleY);
        }
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
                    text: "图片裁剪"
                    color: "#111827"
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: "按比例或指定像素尺寸裁剪图片"
                    color: "#64748b"
                    font.pixelSize: 14
                }
            }
        }

        // Folder selection row
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            radius: 10
            color: "#ffffff"
            border.color: "#e5e9f0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 0
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: 12

                Label {
                    text: "选择文件夹"
                    color: "#475569"
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
                }

                IconButton {
                    iconSource: "qrc:/icons/folder.svg"
                    tooltip: "选择源文件夹"
                    onClicked: sourceFolderDialog.open()
                }

                Rectangle {
                    width: 1
                    height: 24
                    color: "#e2e8f0"
                }

                CheckBox {
                    id: recursiveCheck
                    text: "递归子文件夹"
                    checked: controller ? controller.recursive : false
                    onCheckedChanged: {
                        if (controller && controller.rootPath.length > 0 && checked !== controller.recursive) {
                            // Has images loaded and value actually changed → confirm
                            recursiveConfirmDialog.pendingRecursiveValue = checked;
                            recursiveConfirmDialog.open();
                        } else {
                            // No path set yet or value didn't change, just sync
                            if (controller)
                                controller.recursive = checked;
                        }
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
                color: "#ffffff"
                border.color: "#e5e9f0"
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
                            anchors.fill: parent
                            radius: 6
                            color: root.dispW > 0 && root.dispH > 0 ? "#f1f5f9" : "transparent"

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
                                    source: controller && controller.currentFilePath.length > 0 ? "file:///" + controller.currentFilePath : ""
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

                                    function hitTest(mx, my) {
                                        var ch = cornerHitSize;
                                        // Corners first (higher priority)
                                        if (Math.abs(mx - root.cropX) < ch && Math.abs(my - root.cropY) < ch)
                                            return 2;
                                        if (Math.abs(mx - (root.cropX + root.cropW)) < ch && Math.abs(my - root.cropY) < ch)
                                            return 3;
                                        if (Math.abs(mx - root.cropX) < ch && Math.abs(my - (root.cropY + root.cropH)) < ch)
                                            return 4;
                                        if (Math.abs(mx - (root.cropX + root.cropW)) < ch && Math.abs(my - (root.cropY + root.cropH)) < ch)
                                            return 5;
                                        // Inside crop → move
                                        if (mx >= root.cropX && mx <= root.cropX + root.cropW && my >= root.cropY && my <= root.cropY + root.cropH)
                                            return 1;
                                        return 0;
                                    }

                                    function cursorForMode(mode) {
                                        if (mode === 1)
                                            return Qt.SizeAllCursor;
                                        if (mode === 2)
                                            return Qt.SizeFDiagCursor; // TL
                                        if (mode === 3)
                                            return Qt.SizeBDiagCursor; // TR
                                        if (mode === 4)
                                            return Qt.SizeBDiagCursor; // BL
                                        if (mode === 5)
                                            return Qt.SizeFDiagCursor; // BR
                                        if (!containsMouse)
                                            return Qt.ArrowCursor;
                                        var h = hitTest(mouseX, mouseY);
                                        if (h === 1)
                                            return Qt.SizeAllCursor;
                                        if (h === 2 || h === 5)
                                            return Qt.SizeFDiagCursor;
                                        if (h === 3 || h === 4)
                                            return Qt.SizeBDiagCursor;
                                        return Qt.ArrowCursor;
                                    }

                                    cursorShape: cursorForMode(dragMode > 0 ? dragMode : 0)

                                    onPressed: function (mouse) {
                                        var mode = hitTest(mouse.x, mouse.y);
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
                                            c = root.constrainToRatio(nw, nh);
                                            nw = c.w;
                                            nh = c.h;
                                            maxW = startCropX + startCropW;
                                            maxH = startCropY + startCropH;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                nh = Math.round(nw / root.effectiveRatio()) || nh;
                                            }
                                            if (nh > maxH) {
                                                nh = maxH;
                                                nw = Math.round(nh * root.effectiveRatio()) || nw;
                                            }
                                            root.cropX = maxW - nw;
                                            root.cropY = maxH - nh;
                                            root.cropW = nw;
                                            root.cropH = nh;
                                        } else if (dragMode === 3) {
                                            // TR — fixed corner is BL
                                            nw = startCropW + dx;
                                            nh = startCropH - dy;
                                            c = root.constrainToRatio(nw, nh);
                                            nw = c.w;
                                            nh = c.h;
                                            maxW = root.dispW - startCropX;
                                            maxH = startCropY + startCropH;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                nh = Math.round(nw / root.effectiveRatio()) || nh;
                                            }
                                            if (nh > maxH) {
                                                nh = maxH;
                                                nw = Math.round(nh * root.effectiveRatio()) || nw;
                                            }
                                            root.cropX = startCropX;
                                            root.cropY = maxH - nh;
                                            root.cropW = nw;
                                            root.cropH = nh;
                                        } else if (dragMode === 4) {
                                            // BL — fixed corner is TR
                                            nw = startCropW - dx;
                                            nh = startCropH + dy;
                                            c = root.constrainToRatio(nw, nh);
                                            nw = c.w;
                                            nh = c.h;
                                            maxW = startCropX + startCropW;
                                            maxH = root.dispH - startCropY;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                nh = Math.round(nw / root.effectiveRatio()) || nh;
                                            }
                                            if (nh > maxH) {
                                                nh = maxH;
                                                nw = Math.round(nh * root.effectiveRatio()) || nw;
                                            }
                                            root.cropX = maxW - nw;
                                            root.cropY = startCropY;
                                            root.cropW = nw;
                                            root.cropH = nh;
                                        } else if (dragMode === 5) {
                                            // BR — fixed corner is TL
                                            nw = startCropW + dx;
                                            nh = startCropH + dy;
                                            c = root.constrainToRatio(nw, nh);
                                            nw = c.w;
                                            nh = c.h;
                                            maxW = root.dispW - startCropX;
                                            maxH = root.dispH - startCropY;
                                            if (nw > maxW) {
                                                nw = maxW;
                                                nh = Math.round(nw / root.effectiveRatio()) || nh;
                                            }
                                            if (nh > maxH) {
                                                nh = maxH;
                                                nw = Math.round(nh * root.effectiveRatio()) || nw;
                                            }
                                            root.cropX = startCropX;
                                            root.cropY = startCropY;
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
                                        x: root.cropX - 4
                                        y: root.cropY - 4
                                        width: 8
                                        height: 8
                                        radius: 2
                                        color: "#ffffff"
                                        border.color: "#4f46e5"
                                        border.width: 1.5
                                    }
                                    Rectangle {
                                        x: root.cropX + root.cropW - 4
                                        y: root.cropY - 4
                                        width: 8
                                        height: 8
                                        radius: 2
                                        color: "#ffffff"
                                        border.color: "#4f46e5"
                                        border.width: 1.5
                                    }
                                    Rectangle {
                                        x: root.cropX - 4
                                        y: root.cropY + root.cropH - 4
                                        width: 8
                                        height: 8
                                        radius: 2
                                        color: "#ffffff"
                                        border.color: "#4f46e5"
                                        border.width: 1.5
                                    }
                                    Rectangle {
                                        x: root.cropX + root.cropW - 4
                                        y: root.cropY + root.cropH - 4
                                        width: 8
                                        height: 8
                                        radius: 2
                                        color: "#ffffff"
                                        border.color: "#4f46e5"
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
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "请选择图片文件夹"
                                    color: "#94a3b8"
                                    font.pixelSize: 15
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "支持 PNG、JPG、BMP、WebP、TIFF、GIF 格式"
                                    color: "#c7d2e0"
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
                            color: prevBtnMouse.containsMouse ? "#DDE4EE" : "#B0000000"
                            visible: controller && controller.currentFileCount > 0
                            opacity: prevBtnMouse.containsMouse ? 1.0 : 0.55

                            Behavior on opacity {
                                NumberAnimation {
                                    duration: 150
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                text: "‹"
                                color: prevBtnMouse.containsMouse ? "#1e293b" : "#ffffff"
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
                            color: nextBtnMouse.containsMouse ? "#DDE4EE" : "#B0000000"
                            visible: controller && controller.currentFileCount > 0
                            opacity: nextBtnMouse.containsMouse ? 1.0 : 0.55

                            Behavior on opacity {
                                NumberAnimation {
                                    duration: 150
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                text: "›"
                                color: nextBtnMouse.containsMouse ? "#1e293b" : "#ffffff"
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
                            anchors.top: parent.top
                            anchors.topMargin: 8
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            width: pageLabel.implicitWidth + 16
                            height: 22
                            radius: 11
                            color: "#73000000"
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
                                color: "#ffffff"
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }

                        // ── Crop info label ───────────────────────
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            width: cropInfoLabel.implicitWidth + 16
                            height: 22
                            radius: 6
                            color: "#fafafa"
                            visible: root.cropReady

                            Label {
                                id: cropInfoLabel
                                anchors.centerIn: parent
                                text: {
                                    if (!root.cropReady)
                                        return "";
                                    var ratio = root.effectiveRatio();
                                    if (root.controller && root.controller.cropMode === 0 && ratio > 0) {
                                        return "裁剪比例 " + (ratio >= 1 ? ratio.toFixed(1) : ("1:" + (1 / ratio).toFixed(1)));
                                    } else if (root.controller && root.controller.cropMode === 1) {
                                        return "裁剪尺寸 " + root.controller.targetWidth + " × " + root.controller.targetHeight + " px";
                                    }
                                    return "";
                                }
                                color: "#94a3b8"
                                font.pixelSize: 11
                            }
                        }

                        // ── Image filename label ────────────────────
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: fileNameLabel.implicitWidth + 16
                            height: 22
                            radius: 6
                            color: "#fafafa"
                            visible: root.cropReady && controller && controller.currentFilePath.length > 0

                            Label {
                                id: fileNameLabel
                                anchors.centerIn: parent
                                text: controller ? root.fileNameFromPath(controller.currentFilePath) : ""
                                color: "#94a3b8"
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                                maximumLineCount: 1
                            }
                        }

                        // ── Auto-dismiss Tip (centered in image area) ──
                        Rectangle {
                            id: tipOverlay
                            anchors.centerIn: parent
                            width: tipLabel.implicitWidth + 32
                            height: 36
                            radius: 18
                            color: "#A6000000"
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
                                color: "#ffffff"
                                font.pixelSize: 13
                            }
                        }
                    }
                }
            }

            // ── Right Panel: Controls ──────────────────────────────────
            Rectangle {
                Layout.preferredWidth: 310
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

                    // ── Tab Titles ──────────────────────────────
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            height: 34
                            radius: 8
                            color: controller && controller.cropMode === 0 ? "#4f46e5" : "#f1f5f9"
                            border.color: controller && controller.cropMode === 0 ? "#4f46e5" : "#cbd5e1"
                            border.width: 1

                            Label {
                                anchors.centerIn: parent
                                text: "按比例裁剪"
                                color: controller && controller.cropMode === 0 ? "#ffffff" : "#475569"
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
                            Layout.fillWidth: true
                            height: 34
                            radius: 8
                            color: controller && controller.cropMode === 1 ? "#4f46e5" : "#f1f5f9"
                            border.color: controller && controller.cropMode === 1 ? "#4f46e5" : "#cbd5e1"
                            border.width: 1

                            Label {
                                anchors.centerIn: parent
                                text: "按像素裁剪"
                                color: controller && controller.cropMode === 1 ? "#ffffff" : "#475569"
                                font.pixelSize: 12
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (controller && controller.cropMode !== 1) {
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
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.topMargin: 8
                            radius: 6
                            color: "#f8fafc"
                            border.color: "#e2e8f0"
                            border.width: 1

                            ColumnLayout {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 8
                                spacing: 6

                                Label {
                                    text: "输入设置"
                                    color: "#64748b"
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
                                                property int ratioIdx: index
                                                property bool isSelected: controller && controller.usePresetRatio && controller.presetRatioIndex === ratioIdx
                                                Layout.preferredWidth: 42
                                                Layout.preferredHeight: 26
                                                radius: 5
                                                color: isSelected ? "#6366f1" : "#ffffff"
                                                border.color: isSelected ? "#6366f1" : "#e2e8f0"
                                                border.width: 1

                                                Label {
                                                    anchors.centerIn: parent
                                                    text: modelData
                                                    color: isSelected ? "#ffffff" : "#475569"
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
                                            Layout.preferredWidth: 42
                                            Layout.preferredHeight: 26
                                            radius: 5
                                            color: controller && controller.usePresetRatio && controller.presetRatioIndex === 4 ? "#6366f1" : "#ffffff"
                                            border.color: controller && controller.usePresetRatio && controller.presetRatioIndex === 4 ? "#6366f1" : "#e2e8f0"
                                            border.width: 1

                                            Label {
                                                anchors.centerIn: parent
                                                text: "自由"
                                                color: controller && controller.usePresetRatio && controller.presetRatioIndex === 4 ? "#ffffff" : "#475569"
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

                                        RadioButton {
                                            id: customRatioRadio
                                            text: "自定义"
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
                                            Layout.preferredHeight: 26
                                            placeholderText: "宽"
                                            enabled: controller ? !controller.usePresetRatio : false
                                            text: controller ? controller.customRatioW.toString() : "1"
                                            validator: IntValidator {
                                                bottom: 1
                                                top: 9999
                                            }
                                            horizontalAlignment: TextInput.AlignHCenter
                                            onTextChanged: {
                                                if (controller && !activeFocus) {
                                                    var v = parseInt(text);
                                                    if (!isNaN(v) && v > 0)
                                                        controller.customRatioW = v;
                                                }
                                            }
                                            onActiveFocusChanged: {
                                                if (!activeFocus && controller) {
                                                    var v = parseInt(text);
                                                    if (isNaN(v) || v <= 0)
                                                        v = 1;
                                                    controller.customRatioW = v;
                                                    text = v.toString();
                                                }
                                            }
                                        }

                                        Label {
                                            text: ":"
                                            color: "#94a3b8"
                                            font.pixelSize: 14
                                            font.bold: true
                                        }

                                        TextFieldEx {
                                            Layout.preferredWidth: 50
                                            Layout.preferredHeight: 26
                                            placeholderText: "高"
                                            enabled: controller ? !controller.usePresetRatio : false
                                            text: controller ? controller.customRatioH.toString() : "1"
                                            validator: IntValidator {
                                                bottom: 1
                                                top: 9999
                                            }
                                            horizontalAlignment: TextInput.AlignHCenter
                                            onTextChanged: {
                                                if (controller && !activeFocus) {
                                                    var v = parseInt(text);
                                                    if (!isNaN(v) && v > 0)
                                                        controller.customRatioH = v;
                                                }
                                            }
                                            onActiveFocusChanged: {
                                                if (!activeFocus && controller) {
                                                    var v = parseInt(text);
                                                    if (isNaN(v) || v <= 0)
                                                        v = 1;
                                                    controller.customRatioH = v;
                                                    text = v.toString();
                                                }
                                            }
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
                                            validator: IntValidator {
                                                bottom: 1
                                                top: 99999
                                            }
                                            horizontalAlignment: TextInput.AlignHCenter
                                            onTextChanged: {
                                                if (controller) {
                                                    var v = parseInt(text);
                                                    if (!isNaN(v) && v > 0)
                                                        controller.targetWidth = v;
                                                }
                                            }
                                            onActiveFocusChanged: {
                                                if (!activeFocus && controller) {
                                                    var v = parseInt(text);
                                                    if (isNaN(v) || v <= 0)
                                                        v = 800;
                                                    controller.targetWidth = v;
                                                    text = v.toString();
                                                }
                                            }
                                        }

                                        Label {
                                            text: "×"
                                            color: "#94a3b8"
                                            font.pixelSize: 16
                                        }

                                        TextFieldEx {
                                            id: targetHeightField
                                            Layout.preferredWidth: 70
                                            placeholderText: "高"
                                            text: controller ? controller.targetHeight.toString() : "600"
                                            validator: IntValidator {
                                                bottom: 1
                                                top: 99999
                                            }
                                            horizontalAlignment: TextInput.AlignHCenter
                                            onTextChanged: {
                                                if (controller) {
                                                    var v = parseInt(text);
                                                    if (!isNaN(v) && v > 0)
                                                        controller.targetHeight = v;
                                                }
                                            }
                                            onActiveFocusChanged: {
                                                if (!activeFocus && controller) {
                                                    var v = parseInt(text);
                                                    if (isNaN(v) || v <= 0)
                                                        v = 600;
                                                    controller.targetHeight = v;
                                                    text = v.toString();
                                                }
                                            }
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
                            Layout.fillWidth: true
                            Layout.preferredHeight: 16
                            color: "transparent"

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: 1
                                color: "#e2e8f0"
                            }
                        }

                        // ── Section 3: Output Settings ───────────
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.bottomMargin: 8
                            radius: 6
                            color: "#f8fafc"
                            border.color: "#e2e8f0"
                            border.width: 1

                            ColumnLayout {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 8
                                spacing: 6

                                Label {
                                    text: "输出设置"
                                    color: "#64748b"
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                RowLayout {
                                    spacing: 12

                                    RadioButton {
                                        text: "覆盖源文件"
                                        checked: controller ? controller.outputMode === 0 : true
                                        font.pixelSize: 12
                                        onCheckedChanged: {
                                            if (checked && controller)
                                                controller.outputMode = 0;
                                        }
                                    }

                                    RadioButton {
                                        id: newDirRadio
                                        text: "输出到新目录"
                                        checked: controller ? controller.outputMode === 1 : false
                                        font.pixelSize: 12
                                        onCheckedChanged: {
                                            if (checked && controller)
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
                                        placeholderText: controller ? (controller.rootPath ? controller.rootPath + "_cropped" : "输出到源目录_cropped") : ""
                                        readOnly: false
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
                                    }

                                    IconButton {
                                        iconSource: "qrc:/icons/folder.svg"
                                        tooltip: "选择输出目录"
                                        onClicked: outputFolderDialog.open()
                                    }
                                }
                            }
                        }
                    }

                    // ── Separator ───────────────────────────────
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#f1f5f9"
                        Layout.topMargin: 12
                        Layout.bottomMargin: 12
                    }

                    // ── Action Buttons (fixed at bottom) ────────
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        IconButton {
                            Layout.fillWidth: true
                            text: "执行裁剪"
                            iconSource: "qrc:/icons/play.svg"
                            tooltip: "执行裁剪操作"
                            normalColor: "#4f46e5"
                            hoverColor: "#4338ca"
                            borderColor: "#4338ca"
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
                                controller.executeCrop(controller.cropX, controller.cropY, controller.cropW, controller.cropH);
                            }
                        }
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: statusText.implicitHeight + 12
            radius: 6
            color: "#eff6ff"
            visible: controller ? controller.statusMessage.length > 0 : false

            Label {
                id: statusText
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 12
                text: controller ? controller.statusMessage : ""
                color: "#2563eb"
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }
    }
}
