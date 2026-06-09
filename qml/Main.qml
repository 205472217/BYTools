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
}
