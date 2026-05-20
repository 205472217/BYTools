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

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: homePage
    }

    Component {
        id: homePage

        HomePage {
            onOpenFeature: function(featureId) {
                if (featureId === "rename-converter") {
                    stackView.push(renameConverterPage)
                }
            }
        }
    }

    Component {
        id: renameConverterPage

        RenameConverterPage {
            onBackRequested: stackView.pop()
        }
    }
}
