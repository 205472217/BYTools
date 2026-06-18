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
        id: sourceFolderDialog
        title: "选择源文件夹"
        onAccepted: {
            if (controller) {
                controller.rootPath = selectedFolder
            }
        }
    }

    FolderDialog {
        id: outputFolderDialog
        title: "选择输出目录"
        onAccepted: {
            if (controller) {
                controller.outputDir = selectedFolder
            }
        }
    }

    ColorDialog {
        id: colorDialog
        selectedColor: controller ? controller.bgColor : "#ffffff"
        onAccepted: {
            if (controller) {
                controller.bgColor = selectedColor.toString()
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
                text: "当前有图片格式转换任务正在处理中，返回首页将中断执行，是否继续？"
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
                    text: "图片格式转换"
                    color: "#111827"
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: "批量转换图片格式，支持递归处理子文件夹"
                    color: "#64748b"
                    font.pixelSize: 14
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 310
            radius: 10
            color: "#ffffff"
            border.color: "#e5e9f0"
            border.width: 1

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

                // 行1：源文件夹
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
                        placeholderText: "点击选择文件夹"
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择源文件夹"
                        onClicked: sourceFolderDialog.open()
                    }
                }

                // 行2：目标格式 + 质量
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "目标格式"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    ComboBoxEx {
                        Layout.preferredWidth: 140
                        model: ["PNG", "JPG", "BMP", "WebP", "TIFF"]
                        currentIndex: controller ? controller.targetFormat : 1
                        onActivated: {
                            if (controller) {
                                controller.targetFormat = currentIndex
                            }
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 24
                        color: "#e2e8f0"
                    }

                    Label {
                        text: "质量"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Slider {
                        id: qualitySlider
                        Layout.fillWidth: true
                        Layout.minimumWidth: 80
                        from: 1
                        to: 100
                        stepSize: 1
                        Component.onCompleted: {
                            if (controller) value = controller.quality
                        }
                        onValueChanged: {
                            if (pressed && controller) {
                                controller.quality = Math.round(value)
                            }
                        }
                    }

                    Label {
                        text: (controller ? controller.quality : 85)
                        color: "#111827"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 28
                        horizontalAlignment: Text.AlignRight
                    }
                }

                // 行3：JPG背景色（仅目标格式为JPG时显示）
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true
                    visible: controller ? controller.targetFormat === 1 : false

                    Label {
                        text: "JPG背景色"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RowLayout {
                        spacing: 8

                        Rectangle {
                            width: 28
                            height: 28
                            radius: 6
                            color: "#ffffff"
                            border.width: controller && controller.bgColor === "#ffffff" ? 2 : 1
                            border.color: controller && controller.bgColor === "#ffffff" ? "#2563eb" : "#e2e8f0"

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (controller) controller.bgColor = "#ffffff" }
                            }
                        }

                        Rectangle {
                            width: 28
                            height: 28
                            radius: 6
                            color: "#000000"
                            border.width: controller && controller.bgColor === "#000000" ? 2 : 1
                            border.color: controller && controller.bgColor === "#000000" ? "#2563eb" : "#e2e8f0"

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (controller) controller.bgColor = "#000000" }
                            }
                        }

                        Rectangle {
                            width: 28
                            height: 28
                            radius: 6
                            color: controller ? controller.bgColor : "#ffffff"
                            border.width: 2
                            border.color: "#2563eb"
                            visible: controller
                                     && controller.bgColor !== "#ffffff"
                                     && controller.bgColor !== "#000000"

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: colorDialog.open()
                            }
                        }

                        Label {
                            text: "自定义"
                            color: "#64748b"
                            font.pixelSize: 12

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: colorDialog.open()
                            }
                        }
                    }

                    Label {
                        text: "PNG转JPG时填充透明区域"
                        color: "#94a3b8"
                        font.pixelSize: 12
                    }

                    Item { Layout.fillWidth: true }
                }

                // 行4：输出方式
                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Label {
                        text: "输出方式"
                        color: "#475569"
                        font.pixelSize: 13
                        font.bold: true
                        Layout.preferredWidth: 80
                    }

                    RadioButton {
                        id: replaceRadio
                        text: "替换原文件"
                        checked: controller ? controller.outputMode === 0 : true
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.outputMode = 0
                            }
                        }
                    }

                    RadioButton {
                        id: newDirRadio
                        text: "输出到新目录"
                        checked: controller ? controller.outputMode === 1 : false
                        onCheckedChanged: {
                            if (checked && controller) {
                                controller.outputMode = 1
                            }
                        }
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        text: controller ? controller.outputDir : ""
                        placeholderText: controller ? (controller.rootPath ? controller.rootPath + "_converted" : "自动在源目录后添加 _converted") : ""
                        readOnly: false
                        enabled: newDirRadio.checked
                        onTextChanged: {
                            if (controller) {
                                controller.outputDir = text
                            }
                        }
                    }

                    IconButton {
                        iconSource: "qrc:/icons/folder.svg"
                        tooltip: "选择输出目录"
                        enabled: newDirRadio.checked
                        onClicked: outputFolderDialog.open()
                    }
                }

                // 行5：高级选项
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
                            tooltip: "开始转换"
                            normalColor: "#2563eb"
                            hoverColor: "#1d4ed8"
                            borderColor: "#1d4ed8"
                            onClicked: {
                                if (controller) {
                                    controller.executeConvert()
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

        // 结果列表
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "#ffffff"
            border.color: "#e5e9f0"
            border.width: 1
            clip: true

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
                        text: "格式"
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

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: "#f1f5f9"
                        }

                        // 格式标签
                        Rectangle {
                            x: root.typeColumnX
                            width: root.typeColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: modelData.formatTag && modelData.formatTag === "PNG" ? "#dbeafe" :
                                   modelData.formatTag && modelData.formatTag === "JPG" ? "#d1fae5" :
                                   modelData.formatTag && modelData.formatTag === "BMP" ? "#f3e8ff" :
                                   modelData.formatTag && modelData.formatTag === "WEBP" ? "#fef3c7" :
                                   modelData.formatTag && modelData.formatTag === "TIFF" ? "#fce7f3" :
                                   modelData.formatTag && modelData.formatTag === "GIF" ? "#fef3c7" : "#f1f5f9"

                            Label {
                                anchors.centerIn: parent
                                text: modelData.formatTag
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.formatTag && modelData.formatTag === "PNG" ? "#2563eb" :
                                       modelData.formatTag && modelData.formatTag === "JPG" ? "#0f766e" :
                                       modelData.formatTag && modelData.formatTag === "BMP" ? "#7c3aed" :
                                       modelData.formatTag && modelData.formatTag === "WEBP" ? "#b45309" :
                                       modelData.formatTag && modelData.formatTag === "TIFF" ? "#be185d" :
                                       modelData.formatTag && modelData.formatTag === "GIF" ? "#b45309" : "#64748b"
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
                            text: modelData.status === "已跳过" ? modelData.originalName : modelData.newName
                            color: modelData.status === "已跳过" ? "#94a3b8" : "#059669"
                            font.bold: modelData.status !== "已跳过"
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                        }

                        // 状态标签
                        Rectangle {
                            x: root.statusColumnX
                            width: root.statusColumnWidth
                            y: 14
                            height: 22
                            radius: 4
                            color: modelData.status === "已转换" ? "#dbeafe" :
                                   modelData.status === "已跳过" ? "#f1f5f9" : "#fef2f2"

                            Label {
                                anchors.centerIn: parent
                                text: modelData.status
                                font.pixelSize: 11
                                font.bold: true
                                color: modelData.status === "已转换" ? "#2563eb" :
                                       modelData.status === "已跳过" ? "#64748b" : "#dc2626"
                                elide: Text.ElideRight
                            }
                        }

                        Label {
                            x: root.originalColumnX
                            y: 40
                            width: root.statusColumnX - root.originalColumnX - root.columnGap
                            text: modelData.success ? modelData.newPath : modelData.originalPath
                            color: "#94a3b8"
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
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
                            text: "暂无转换记录"
                            color: "#94a3b8"
                            font.pixelSize: 15
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "设置参数后点击执行按钮开始"
                            color: "#c7d2e0"
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}
