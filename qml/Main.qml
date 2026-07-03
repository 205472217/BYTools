import QtQuick
import QtQuick.Controls
import BYTools

ApplicationWindow {
    id: window

    property var pal: themeManager.palette

    width: 1120
    height: 720
    minimumWidth: 1120
    minimumHeight: 720
    visible: true
    title: "BYTools"
    color: pal.SurfaceEx_pageBg

    property string currentFeatureId: ""
    property bool _navGuard: false
    property bool _forceClosing: false

    // 关闭保护：有任务执行时确认是否退出
    onClosing: function(closeEvent) {
        if (_forceClosing) {
            _forceClosing = false
            return
        }
        if (pluginManager.hasProcessingTasks()) {
            closeEvent.accepted = false
            confirmCloseDialog.open()
        }
    }

    ConfirmDialog {
        id: confirmCloseDialog
        dialogTitle: "确认退出"
        messageText: "有任务正在执行中，确定要退出吗？\n退出后任务将被中断。"
        onConfirmed: {
            window._forceClosing = true
            Qt.quit()
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: homePage

        onBusyChanged: {
            if (!busy)
                window._navGuard = false
        }
    }

    // ── 启动加载遮罩：mpv 解压中 ──
    Rectangle {
        id: loadingOverlay
        anchors.fill: stackView
        z: stackView.z + 2
        color: pal.SurfaceEx_pageBg
        visible: pluginManager.mpvExtracting

        Column {
            anchors.centerIn: parent
            spacing: 24

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 48
                height: 48
                running: parent.visible
            }

            Label {
                text: "正在初始化 mpv 播放器..."
                anchors.horizontalCenter: parent.horizontalCenter
                color: pal.LabelEx_infoText
                font.pixelSize: 15
            }

            Label {
                text: "首次启动需解压播放器组件，请稍候"
                anchors.horizontalCenter: parent.horizontalCenter
                color: pal.LabelEx_subtitleText
                font.pixelSize: 12
            }
        }
    }

    // 页面切换遮罩：防止切换过程中点击穿透
    Rectangle {
        id: transitionOverlay
        anchors.fill: stackView
        z: stackView.z + 1
        color: pal.SurfaceEx_overlay
        visible: stackView.busy

        MouseArea {
            anchors.fill: parent
            enabled: parent.visible
        }
    }

    function navigateBack() {
        if (stackView.busy || window._navGuard) return
        window._navGuard = true
        stackView.pop()
        window.currentFeatureId = ""
    }

    Component {
        id: homePage

        HomePage {
            onOpenFeature: function(featureId) {
                if (window._navGuard || window.currentFeatureId === featureId || stackView.busy) return
                window._navGuard = true

                var controller = pluginManager.getPlugin(featureId)
                if (!controller) { window._navGuard = false; return }

                window.currentFeatureId = featureId

                var qmlUrl = Qt.resolvedUrl("pages/" + pluginManager.pluginQmlUrl(featureId))
                var page = stackView.push(qmlUrl, {controller: controller, stackView: stackView, pluginId: featureId})
                page.backRequested.connect(navigateBack)
            }
        }
    }
}
