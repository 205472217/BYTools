import QtQuick
import QtQuick.Controls
import BYTools

ApplicationWindow {
    id: window

    width: 1120
    height: 720
    minimumWidth: 880
    minimumHeight: 560
    visible: true
    title: "BYTools"
    color: "#f4f6f9"

    property string currentFeatureId: ""

    // 关闭保护：有任务执行时确认是否退出
    onClosing: function(closeEvent) {
        var pluginIds = ["name-converter", "batch-rename", "image-converter",
                         "image-crop", "video-subtitle", "custom-subtitle",
                         "subtitle-adjust"]
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

        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
                easing.type: Easing.OutCubic
            }
            PropertyAnimation {
                property: "x"
                from: stackView.width * 0.08
                to: 0
                duration: 250
                easing.type: Easing.OutCubic
            }
        }
        pushExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
                easing.type: Easing.InCubic
            }
        }
        popEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
                easing.type: Easing.InCubic
            }
        }
    }

    Component {
        id: homePage

        HomePage {
            onOpenFeature: function(featureId) {
                var controller = pluginManager.getPlugin(featureId)
                if (!controller) return

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
                }
            }
        }
    }

    Component {
        id: nameConverterPageComponent

        NameConverterPage {
            onBackRequested: {
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: batchRenamePageComponent

        BatchRenamePage {
            onBackRequested: {
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: imageConverterPageComponent

        ImageConverterPage {
            onBackRequested: {
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: imageCropPageComponent

        ImageCropPage {
            onBackRequested: {
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: videoSubtitlePageComponent

        VideoSubtitlePage {
            onBackRequested: {
                stackView.pop()
                window.currentFeatureId = ""
            }
            onOpenSettings: {
                stackView.push(videoSubtitleSettingsPageComponent)
            }
        }
    }

    Component {
        id: videoSubtitleSettingsPageComponent

        VideoSubtitleSettingsPage {
            settings: pluginManager.getPluginSettings("video-subtitle")
            onBackRequested: {
                stackView.pop()
            }
        }
    }

    Component {
        id: customSubtitlePageComponent

        CustomSubtitlePage {
            onBackRequested: {
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: subtitleAdjustPageComponent

        SubtitleAdjustPage {
            onBackRequested: {
                stackView.pop()
                window.currentFeatureId = ""
            }
        }
    }
}
