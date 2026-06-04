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
    readonly property int textColumnWidth: Math.max(160, (recordsListView.width
        - tableLeftPadding
        - tableRightPadding
        - typeColumnWidth
        - statusColumnWidth
        - actionColumnWidth
        - columnGap * 4) / 2)
    readonly property int typeColumnX: tableLeftPadding
    readonly property int originalColumnX: typeColumnX + typeColumnWidth + columnGap
    readonly property int newColumnX: originalColumnX + textColumnWidth + columnGap
    readonly property int statusColumnX: newColumnX + textColumnWidth + columnGap
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
        title: "选择根文件夹"
        onAccepted: {
            if (controller) {
                controller.rootPath = selectedFolder
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
                onClicked: root.backRequested()
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: "批量重命名"
                    color: "#111827"
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: "设置规则后直接执行，支持逐条还原"
                    color: "#64748b"
                    font.pixelSize: 14
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 280
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
                        text: "根文件夹"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.rootPath : ""
                        readOnly: true
                        placeholderText: "点击选择文件夹"
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择根文件夹"
                        onClicked: folderDialog.open()
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "文件类型"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        RadioButton {
                            text: "所有"
                            checked: controller ? controller.fileType === 0 : true
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 0
                                }
                            }
                        }

                        RadioButton {
                            text: "视频"
                            checked: controller ? controller.fileType === 1 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 1
                                }
                            }
                        }

                        RadioButton {
                            text: "音频"
                            checked: controller ? controller.fileType === 2 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 2
                                }
                            }
                        }

                        RadioButton {
                            text: "文本"
                            checked: controller ? controller.fileType === 3 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 3
                                }
                            }
                        }

                        RadioButton {
                            text: "图片"
                            checked: controller ? controller.fileType === 4 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 4
                                }
                            }
                        }

                        RadioButton {
                            id: customRadio
                            text: "自定义"
                            checked: controller ? controller.fileType === 5 : false
                            onCheckedChanged: {
                                if (checked && controller) {
                                    controller.fileType = 5
                                }
                            }
                        }

                        TextFieldEx {
                            Layout.preferredWidth: 80
                            text: controller ? controller.customExtension : ""
                            placeholderText: ".txt"
                            enabled: controller ? controller.fileType === 5 : false
                            onTextChanged: {
                                if (controller) {
                                    controller.customExtension = text
                                }
                            }
                        }

                        Label {
                            text: controller ? controller.fileTips : ""
                            color: "#64748b"
                            font.pixelSize: 12
                            visible: controller ? controller.fileType !== 5 : false
                        }
                    }
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "重命名方式"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RadioButton {
                        id: specifyRadio
                        text: "指定名称"
                        checked: controller ? controller.renameMode === 0 : true
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.renameMode = 0
                            }
                        }
                    }

                    TextFieldEx {
                        Layout.preferredWidth: 150
                        text: controller ? controller.baseName : ""
                        placeholderText: "输入文件名"
                        enabled: specifyRadio.checked
                        onTextChanged: {
                            if (controller) {
                                controller.baseName = text
                            }
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 24
                        color: "#e2e8f0"
                    }

                    RadioButton {
                        id: replaceRadio
                        text: "替换文本"
                        checked: controller ? controller.renameMode === 1 : false
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.renameMode = 1
                            }
                        }
                    }

                    TextFieldEx {
                        Layout.preferredWidth: 120
                        text: controller ? controller.searchText : ""
                        placeholderText: "查找"
                        enabled: replaceRadio.checked
                        onTextChanged: {
                            if (controller) {
                                controller.searchText = text
                            }
                        }
                    }

                    Label {
                        text: "→"
                        color: "#64748b"
                    }

                    TextFieldEx {
                        Layout.preferredWidth: 120
                        text: controller ? controller.replaceText : ""
                        placeholderText: "替换"
                        enabled: replaceRadio.checked
                        onTextChanged: {
                            if (controller) {
                                controller.replaceText = text
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
                            tooltip: "开始执行"
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            visible: (controller && controller.statusMessage.length > 0) || (controller && controller.hasRecords)

            Rectangle {
                Layout.fillWidth: true
                height: statusText.implicitHeight + 12
                radius: 6
                color: controller && controller.statusMessage ? "#eff6ff" : "transparent"
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

            IconButton {
                iconSource: "qrc:/icons/undo.svg"
                tooltip: "批量还原"
                visible: controller ? controller.hasRecords : false
                onClicked: {
                    if (controller) {
                        controller.restoreAllRecords()
                    }
                }
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
                    radius: 0

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
                        x: root.originalColumnX
                        width: root.textColumnWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: "原名称"
                        color: "#64748b"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        x: root.newColumnX
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
                    id: recordsListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 200
                    model: controller ? controller.records : null
                    clip: true
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AlwaysOn
                    }

                    delegate: Rectangle {
                        id: rowDelegate
                        width: recordsListView.width
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
                            color: modelData.fileType.startsWith("视频") ? "#f3e8ff" :
                                   modelData.fileType.startsWith("图片") ? "#d1fae5" :
                                   modelData.fileType.startsWith("音频") ? "#fef3c7" :
                                   modelData.fileType.startsWith("文本") ? "#dbeafe" : "#f1f5f9"

                            Label {
                                anchors.centerIn: parent
                                text: modelData.fileType
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.fileType.startsWith("视频") ? "#7c3aed" :
                                       modelData.fileType.startsWith("图片") ? "#0f766e" :
                                       modelData.fileType.startsWith("音频") ? "#b45309" :
                                       modelData.fileType.startsWith("文本") ? "#2563eb" : "#64748b"
                            }
                        }

                        Label {
                            x: root.originalColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: modelData.originalName
                            color: "#334155"
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        Label {
                            x: root.newColumnX
                            width: root.textColumnWidth
                            y: 14
                            text: modelData.newName
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
                            color: modelData.success ? "#dbeafe" : "#fef2f2"

                            Label {
                                anchors.centerIn: parent
                                text: modelData.status
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.success ? "#2563eb" : "#dc2626"
                            }
                        }

                        Label {
                            x: root.originalColumnX
                            y: 40
                            width: root.statusColumnX - root.originalColumnX - root.columnGap
                            text: (modelData.success ? modelData.newPath : modelData.originalPath)
                            color: "#94a3b8"
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }

                        IconButton {
                            x: root.actionColumnX
                            width: root.actionColumnWidth
                            anchors.verticalCenter: parent.verticalCenter
                            iconSource: "qrc:/icons/undo.svg"
                            tooltip: modelData.success ? "还原" : "失败项无法还原"
                            enabled: modelData.success
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
                        visible: controller ? recordsListView.count === 0 : true

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "暂无重命名记录"
                            color: "#94a3b8"
                            font.pixelSize: 15
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "设置规则后点击执行按钮开始"
                            color: "#c7d2e0"
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}