import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Pane {
    id: root

    signal backRequested()

    property var controller: null

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

    Component.onDestruction: {
        if (controller && typeof controller.reset === 'function') {
            controller.reset()
        }
    }

    padding: 0
    background: Rectangle {
        color: "#f4f6f9"
    }

    FolderDialog {
        id: folderDialog
        title: "选择源文件夹"
        onAccepted: {
            if (controller) {
                controller.rootPath = selectedFolder
                controller.clearRecords()
            }
        }
    }

    // ── 任务执行中返回确认对话框 ─────────────────────────────────────
    Dialog {
        id: backConfirmDialog
        title: "确认返回"
        modal: true
        anchors.centerIn: parent
        width: 400
        standardButtons: Dialog.NoButton
        closePolicy: Dialog.CloseOnEscape

        contentItem: ColumnLayout {
            spacing: 8
            Layout.margins: 4

            Label {
                text: "当前有繁转简任务正在处理中，返回首页将中断执行，是否继续？"
                color: "#334155"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.bottomMargin: 8
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Item { Layout.fillWidth: true }

                IconButton {
                    text: "取消"
                    tooltip: "不返回，继续当前任务"
                    normalColor: "#e2e8f0"
                    hoverColor: "#cbd5e1"
                    borderColor: "#cbd5e1"
                    textColor: "#475569"
                    implicitWidth: 100
                    implicitHeight: 38
                    onClicked: backConfirmDialog.close()
                }

                IconButton {
                    text: "返回首页"
                    tooltip: "中断任务并返回首页"
                    normalColor: "#dc2626"
                    hoverColor: "#b91c1c"
                    borderColor: "#b91c1c"
                    textColor: "#ffffff"
                    implicitWidth: 120
                    implicitHeight: 38
                    onClicked: {
                        if (controller) { controller.cancel(); }
                        backConfirmDialog.close();
                        root.backRequested();
                    }
                }
            }
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
                iconSource: "qrc:/icons/arrow-left.svg"
                tooltip: "返回"
                onClicked: {
                    if (controller && controller.isProcessing) {
                        backConfirmDialog.open();
                    } else {
                        root.backRequested();
                    }
                }
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
                    text: "选择源文件夹后直接执行转换，完成后可按记录逐条还原"
                    color: "#64748b"
                    font.pixelSize: 14
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 174
            radius: 10
            color: "#ffffff"
            border.color: "#e5e9f0"
            border.width: 1

            // 面板阴影
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
                spacing: 12

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "源文件夹"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.rootPath : ""
                        readOnly: true
                        placeholderText: "尚未选择"
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择源文件夹"
                        onClicked: folderDialog.open()
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "处理类型"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    ComboBoxEx {
                        Layout.fillWidth: true
                        model: ["仅文件", "仅文件夹", "文件和文件夹"]
                        currentIndex: controller ? controller.targetType : 2
                        onActivated: {
                            if (controller) {
                                controller.targetType = currentIndex
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "高级选项"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    CheckBox {
                        text: "递归处理子文件夹"
                        checked: controller ? controller.recursive : false
                        onCheckedChanged: {
                            if (controller) {
                                controller.recursive = checked
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    RowLayout {
                        spacing: 8

                        IconButton {
                            iconSource: "qrc:/icons/trash.svg"
                            tooltip: "清空记录"
                            visible: controller ? controller.hasRecords : false
                            onClicked: {
                                if (controller) {
                                    controller.clearRecords()
                                }
                            }
                        }

                        IconButton {
                            text: "执行"
                            iconSource: "qrc:/icons/play.svg"
                            tooltip: "执行转换"
                            normalColor: "#2563eb"
                            hoverColor: "#1d4ed8"
                            borderColor: "#1d4ed8"
                            onClicked: {
                                if (controller) {
                                    controller.executeRename()
                                }
                            }
                        }
                    }
                }
            }
        }

        // 状态栏
        Rectangle {
            Layout.fillWidth: true
            height: statusText.implicitHeight + 12
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

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "#ffffff"
            border.color: "#e5e9f0"
            border.width: 1
            clip: true

            // 面板阴影
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
                anchors.margins: 2
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: "#f8fafc"

                    // 底部分隔线
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: "#e8ecf2"
                    }

                    Label {
                        x: root.typeColumnX
                        width: root.typeColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "类型"
                        color: "#64748b"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        x: root.currentNameColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "原名称"
                        color: "#64748b"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        x: root.newNameColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "新名称"
                        color: "#64748b"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        x: root.statusColumnX
                        width: root.statusColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "状态"
                        color: "#64748b"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        x: root.actionColumnX
                        width: root.actionColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "操作"
                        color: "#64748b"
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                ListView {
                    id: previewList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: controller ? controller.previewModel() : null
                    clip: true
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AlwaysOn
                    }

                    delegate: Rectangle {
                        id: rowDelegate
                        width: previewList.width
                        height: 74
                        color: rowMouseArea.containsMouse ? "#f0f7ff" :
                               index % 2 === 0 ? "#ffffff" : "#fafbfc"

                        // 底部分隔线
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: "#f1f5f9"
                        }

                        // 类型标签（彩色标签）
                        Rectangle {
                            x: root.typeColumnX
                            width: root.typeColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: directory ? "#f3e8ff" : "#d1fae5"

                            Label {
                                anchors.centerIn: parent
                                text: directory ? "文件夹" : "文件"
                                font.pixelSize: 11
                                font.bold: true
                                color: directory ? "#7c3aed" : "#0f766e"
                            }
                        }

                        Label {
                            x: root.currentNameColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: currentName
                            color: "#334155"
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        Label {
                            x: root.newNameColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: newName
                            color: "#059669"
                            font.bold: true
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        // 状态标签（彩色标签）
                        Rectangle {
                            x: root.statusColumnX
                            width: root.statusColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: status.indexOf("失败") === 0 ? "#fef2f2" :
                                   status === "已还原" ? "#f1f5f9" : "#dbeafe"

                            Label {
                                anchors.centerIn: parent
                                text: status
                                font.pixelSize: 11
                                font.bold: true
                                color: status.indexOf("失败") === 0 ? "#dc2626" :
                                       status === "已还原" ? "#64748b" : "#2563eb"
                                elide: Text.ElideRight
                            }
                        }

                        Label {
                            x: root.currentNameColumnX
                            y: 40
                            width: root.statusColumnX - root.currentNameColumnX - root.columnGap
                            text: actualPath
                            color: "#94a3b8"
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }

                        IconButton {
                            x: root.actionColumnX
                            width: root.actionColumnWidth
                            anchors.verticalCenter: parent.verticalCenter
                            iconSource: "qrc:/icons/undo.svg"
                            tooltip: status.indexOf("失败") === 0 ? "失败项无法还原" : "还原"
                            enabled: status === "已转换"
                            onClicked: {
                                if (controller) {
                                    controller.restoreRecord(index)
                                }
                            }
                        }

                        MouseArea {
                            id: rowMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    // 空状态
                    Column {
                        anchors.centerIn: parent
                        spacing: 8
                        visible: previewList.count === 0

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "暂无转换记录"
                            color: "#94a3b8"
                            font.pixelSize: 15
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "选择文件夹后点击执行按钮开始"
                            color: "#c7d2e0"
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}