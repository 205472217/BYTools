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
    color: "#f6f7f9"

    property var currentController: null
    property string currentFeatureId: ""

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: homePage
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