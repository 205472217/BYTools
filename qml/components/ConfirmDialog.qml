//ConfirmDialog
import QtQuick
import QtQuick.Controls

Dialog {
    id: root

    property string dialogTitle: "确认"
    property string messageText: ""
    property bool forceClosing: false

    title: dialogTitle
    modal: true
    anchors.centerIn: parent
    width: 420
    standardButtons: Dialog.Ok | Dialog.Cancel

    signal confirmed()

    contentItem: Label {
        text: messageText
        color: themeManager.palette.LabelEx_statusText
        font.pixelSize: 14
        wrapMode: Text.WordWrap
        focus: true
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                root.accept()
                event.accepted = true
            }
        }
    }

    onAccepted: {
        root.confirmed()
    }
}
