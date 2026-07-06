//ConfirmDialog
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: root

    property string dialogTitle: "确认"
    property string messageText: ""
    property bool forceClosing: false

    title: dialogTitle
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    modality: Qt.ApplicationModal

    width: 420
    height: contentColumn.implicitHeight + 48
    minimumWidth: 300
    minimumHeight: 100

    signal confirmed
    signal opened
    signal closed

    function open() {
        x = (Screen.width - width) / 2
        y = (Screen.height - height) / 2
        show()
        opened()
        focusItem.forceActiveFocus()
    }

    function close() {
        hide()
    }

    function accept() {
        confirmed()
        close()
    }

    function reject() {
        close()
    }

    onVisibleChanged: {
        if (!visible) closed()
    }

    Item {
        id: focusItem
        anchors.fill: parent
        focus: true

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                root.accept()
                event.accepted = true
            }
            if (event.key === Qt.Key_Escape) {
                root.reject()
                event.accepted = true
            }
        }

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 16
            spacing: 16

            Label {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: messageText
                color: themeManager.palette.LabelEx_statusText
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8

                Button {
                    text: "取消"
                    onClicked: root.reject()
                }

                Button {
                    text: "确定"
                    onClicked: root.accept()
                }
            }
        }
    }
}
