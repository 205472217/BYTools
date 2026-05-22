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

    property var currentController: null
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
            PropertyAnimation {
                property: "x"
                from: 0
                to: stackView.width * 0.08
                duration: 200
                easing.type: Easing.InCubic
            }
        }
    }

    Component {
        id: homePage

        HomePage {
            onOpenFeature: function(featureId) {
                var controller = pluginManager.getPlugin(featureId)
                if (controller) {
                    window.currentController = controller
                    window.currentFeatureId = featureId
                }

                if (featureId === "name-converter") {
                    stackView.push(nameConverterPage)
                } else if (featureId === "batch-rename") {
                    stackView.push(batchRenamePage)
                }
            }
        }
    }

    Component {
        id: nameConverterPage

        NameConverterPage {
            controller: window.currentController
            onBackRequested: {
                stackView.pop()
                window.currentController = null
                window.currentFeatureId = ""
            }
        }
    }

    Component {
        id: batchRenamePage

        BatchRenamePage {
            controller: window.currentController
            onBackRequested: {
                stackView.pop()
                window.currentController = null
                window.currentFeatureId = ""
            }
        }
    }
}