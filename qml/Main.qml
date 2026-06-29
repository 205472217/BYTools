import QtQuick
import QtQuick.Controls
import BYTools

ApplicationWindow {
    id: window

    property var pal: themeManager.palette

    width: 1120
    height: 720
    minimumWidth: 1120
    minimumHeight: 720
    visible: true
    title: "BYTools"
    color: pal.SurfaceEx_pageBg

    property string currentFeatureId: ""
    property bool _navGuard: false

    // 关闭保护：有任务执行时确认是否退出
    onClosing: function(closeEvent) {
        var pluginIds = ["name-converter", "batch-rename", "image-converter",
                         "image-crop", "video-subtitle", "custom-subtitle",
                         "subtitle-adjust", "file-view"]
        var anyProcessing = false
        for (var i = 0; i < pluginIds.length; i++) {
            var ctrl = pluginManager.getPlugin(pluginIds[i])
            if (ctrl && ctrl.isProcessing) {
                anyProcessing = true
                break
            }
        }
        if (anyProcessing) {
            closeEvent.accepted = false
            confirmCloseDialog.open()
        }
    }

    Dialog {
        id: confirmCloseDialog
        title: "确认退出"
        standardButtons: Dialog.Yes | Dialog.No
        closePolicy: Popup.CloseOnEscape
        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        modal: true

        Label {
            text: "有任务正在执行中，确定要退出吗？\n退出后任务将被中断。"
            wrapMode: Text.WordWrap
            width: 320
            lineHeight: 1.5
        }

        onAccepted: Qt.quit()
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: homePage

        onBusyChanged: {
            if (!busy)
                window._navGuard = false
        }
    }

    // ── 启动加载遮罩：mpv 解压中 ──
    Rectangle {
        id: loadingOverlay
        anchors.fill: stackView
        z: stackView.z + 2
        color: pal.SurfaceEx_pageBg
        visible: pluginManager.mpvExtracting

        Column {
            anchors.centerIn: parent
            spacing: 24

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 48
                height: 48
                running: parent.visible
            }

            Label {
                text: "正在初始化 mpv 播放器..."
                anchors.horizontalCenter: parent.horizontalCenter
                color: pal.LabelEx_infoText
                font.pixelSize: 15
            }

            Label {
                text: "首次启动需解压播放器组件，请稍候"
                anchors.horizontalCenter: parent.horizontalCenter
                color: pal.LabelEx_subtitleText
                font.pixelSize: 12
            }
        }
    }

    // 页面切换遮罩：防止切换过程中点击穿透
    Rectangle {
        id: transitionOverlay
        anchors.fill: stackView
        z: stackView.z + 1
        color: "#40000000"
        visible: stackView.busy

        MouseArea {
            anchors.fill: parent
            enabled: parent.visible
        }
    }

    Component {
        id: homePage

        HomePage {
            onOpenFeature: function(featureId) {
                if (window._navGuard || window.currentFeatureId === featureId || stackView.busy) return
                window._navGuard = true

                var controller = pluginManager.getPlugin(featureId)
                if (!controller) { window._navGuard = false; return }

                window.currentFeatureId = featureId

                if (featureId === "name-converter") {
                    stackView.push(nameConverterPageComponent, {controller: controller})
                } else if (featureId === "batch-rename") {
                    stackView.push(batchRenamePageComponent, {controller: controller})
                } else if (featureId === "image-converter") {
                    stackView.push(imageConverterPageComponent, {controller: controller})
                } else if (featureId === "image-crop") {
                    stackView.push(imageCropPageComponent, {controller: controller})
                } else if (featureId === "video-subtitle") {
                    stackView.push(videoSubtitlePageComponent, {controller: controller})
                } else if (featureId === "custom-subtitle") {
                    stackView.push(customSubtitlePageComponent, {controller: controller})
                } else if (featureId === "subtitle-adjust") {
                    stackView.push(subtitleAdjustPageComponent, {controller: controller})
                } else if (featureId === "file-view") {
                    stackView.push(fileViewPageComponent, {controller: controller})
                }
            }
        }
    }

    Component {
        id: nameConverterPageComponent

        NameConverterPage {
            onBackRequested: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: batchRenamePageComponent

        BatchRenamePage {
            onBackRequested: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: imageConverterPageComponent

        ImageConverterPage {
            onBackRequested: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: imageCropPageComponent

        ImageCropPage {
            onBackRequested: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: videoSubtitlePageComponent

        VideoSubtitlePage {
            onBackRequested: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.pop()
                window.currentFeatureId = ""
            }
            onOpenSettings: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.push(videoSubtitleSettingsPageComponent)
            }
        }
    }

    Component {
        id: videoSubtitleSettingsPageComponent

        VideoSubtitleSettingsPage {
            onBackRequested: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.pop()
            }
        }
    }

    Component {
        id: customSubtitlePageComponent

        CustomSubtitlePage {
            onBackRequested: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: subtitleAdjustPageComponent

        SubtitleAdjustPage {
            onBackRequested: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: fileViewPageComponent

        FileViewPage {
            onBackRequested: {
                if (stackView.busy || window._navGuard) return
                window._navGuard = true
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }
}
