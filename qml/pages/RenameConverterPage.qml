import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Pane {
    id: root

    signal backRequested()

    readonly property int tableLeftPadding: 18
    readonly property int tableRightPadding: 20
    readonly property int typeColumnWidth: 74
    readonly property int statusColumnWidth: 118
    readonly property int actionColumnWidth: 44
    readonly property int columnGap: 12
    readonly property int textColumnWidth: Math.max(160, (previewList.width
        - tableLeftPadding
        - tableRightPadding
        - typeColumnWidth
        - statusColumnWidth
        - actionColumnWidth
        - columnGap * 4) / 2)
    readonly property int typeColumnX: tableLeftPadding
    readonly property int currentNameColumnX: typeColumnX + typeColumnWidth + columnGap
    readonly property int newNameColumnX: currentNameColumnX + textColumnWidth + columnGap
    readonly property int statusColumnX: newNameColumnX + textColumnWidth + columnGap
    readonly property int actionColumnX: statusColumnX + statusColumnWidth + columnGap

    padding: 0
    background: Rectangle {
        color: "#f6f7f9"
    }

    FolderDialog {
        id: folderDialog
        title: "选择根文件夹"
        onAccepted: {
            batchRenameController.rootPath = selectedFolder
            batchRenameController.clearRecords()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            IconButton {
                iconSource: "../icons/arrow-left.svg"
                tooltip: "返回"
                onClicked: root.backRequested()
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: "文件名繁转简"
                    color: "#111827"
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: "选择根文件夹后直接执行转换，完成后可按记录逐条还原"
                    color: "#64748b"
                    font.pixelSize: 14
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 136
            radius: 8
            color: "#ffffff"
            border.color: "#d9dde5"

            GridLayout {
                anchors.fill: parent
                anchors.margins: 18
                columns: 3
                columnSpacing: 14
                rowSpacing: 12

                Label {
                    text: "根文件夹"
                    color: "#334155"
                    font.pixelSize: 14
                }

                TextField {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    text: batchRenameController.rootPath
                    readOnly: true
                    placeholderText: "尚未选择"
                    verticalAlignment: Text.AlignVCenter
                }

                IconButton {
                    iconSource: "../icons/folder-open.svg"
                    tooltip: "选择根文件夹"
                    onClicked: folderDialog.open()
                }

                Label {
                    text: "处理类型"
                    color: "#334155"
                    font.pixelSize: 14
                }

                ComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    model: ["仅文件", "仅文件夹", "文件和文件夹"]
                    currentIndex: batchRenameController.targetType
                    onActivated: batchRenameController.targetType = currentIndex
                }

                RowLayout {
                    spacing: 10
                    Layout.preferredWidth: 86

                    IconButton {
                        iconSource: "../icons/play.svg"
                        tooltip: "执行转换"
                        normalColor: "#2563eb"
                        hoverColor: "#1d4ed8"
                        borderColor: "#1d4ed8"
                        onClicked: batchRenameController.executeRename()
                    }

                    Item {
                        width: 38
                        height: 38

                        IconButton {
                            anchors.centerIn: parent
                            iconSource: "../icons/trash.svg"
                            tooltip: "清空记录"
                            visible: batchRenameController.hasRecords
                            onClicked: batchRenameController.clearRecords()
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: batchRenameController.statusMessage
            color: "#2563eb"
            font.pixelSize: 14
            elide: Text.ElideRight
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: "#ffffff"
            border.color: "#d9dde5"
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 2
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: "#f8fafc"

                    Label {
                        x: root.typeColumnX
                        width: root.typeColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "类型"
                        color: "#475569"
                        font.bold: true
                    }

                    Label {
                        x: root.currentNameColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "原名称"
                        color: "#475569"
                        font.bold: true
                    }

                    Label {
                        x: root.newNameColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "新名称"
                        color: "#475569"
                        font.bold: true
                    }

                    Label {
                        x: root.statusColumnX
                        width: root.statusColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "状态"
                        color: "#475569"
                        font.bold: true
                    }

                    Label {
                        x: root.actionColumnX
                        width: root.actionColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "操作"
                        color: "#475569"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                ListView {
                    id: previewList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: renamePreviewModel
                    clip: true

                    delegate: Rectangle {
                        width: previewList.width
                        height: 74
                        color: index % 2 === 0 ? "#ffffff" : "#fbfcfe"

                        Label {
                            x: root.typeColumnX
                            width: root.typeColumnWidth
                            y: 12
                            text: directory ? "文件夹" : "文件"
                            color: directory ? "#7c3aed" : "#0f766e"
                        }

                        Label {
                            x: root.currentNameColumnX
                            width: root.textColumnWidth
                            y: 12
                            text: currentName
                            color: "#1f2937"
                            elide: Text.ElideMiddle
                        }

                        Label {
                            x: root.newNameColumnX
                            width: root.textColumnWidth
                            y: 12
                            text: newName
                            color: "#1f2937"
                            elide: Text.ElideMiddle
                            font.bold: true
                        }

                        Label {
                            x: root.statusColumnX
                            width: root.statusColumnWidth
                            y: 12
                            text: status
                            color: status.indexOf("失败") === 0 ? "#dc2626" : status === "已还原" ? "#64748b" : "#2563eb"
                            elide: Text.ElideRight
                        }

                        Label {
                            x: root.currentNameColumnX
                            y: 42
                            width: root.statusColumnX - root.currentNameColumnX - root.columnGap
                            text: "路径：" + actualPath
                            color: "#64748b"
                            font.pixelSize: 12
                            elide: Text.ElideMiddle
                        }

                        IconButton {
                            x: root.actionColumnX
                            width: root.actionColumnWidth
                            anchors.verticalCenter: parent.verticalCenter
                            iconSource: "../icons/undo.svg"
                            tooltip: status.indexOf("失败") === 0 ? "失败项无法还原" : "还原"
                            enabled: status === "已转换"
                            onClicked: batchRenameController.restoreRecord(index)
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: previewList.count === 0
                        text: "暂无转换记录"
                        color: "#94a3b8"
                        font.pixelSize: 16
                    }
                }
            }
        }
    }
}