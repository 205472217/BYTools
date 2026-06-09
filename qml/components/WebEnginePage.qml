import QtQuick
import QtQuick.Layouts
import QtWebEngine

Rectangle {
    id: root

    property string downloadPath: ""
    signal downloadRequested(string url, string fileName)

    color: "#f8fafc"
    border.color: "#e2e8f0"
    border.width: 1
    radius: 6

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Navigation bar ──
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: "#f1f5f9"
            radius: 6

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 4

                Rectangle {
                    implicitWidth: 26; implicitHeight: 26; radius: 4
                    color: bMouse.containsMouse ? "#e2e8f0" : "transparent"
                    property bool canNav: webView.canGoBack
                    opacity: canNav ? 1.0 : 0.4

                    Label {
                        anchors.centerIn: parent; text: "◀"
                        font.pixelSize: 12; color: "#475569"
                    }
                    MouseArea {
                        id: bMouse; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (parent.canNav) webView.goBack()
                    }
                }

                Rectangle {
                    implicitWidth: 26; implicitHeight: 26; radius: 4
                    color: fMouse.containsMouse ? "#e2e8f0" : "transparent"
                    property bool canNav: webView.canGoForward
                    opacity: canNav ? 1.0 : 0.4

                    Label {
                        anchors.centerIn: parent; text: "▶"
                        font.pixelSize: 12; color: "#475569"
                    }
                    MouseArea {
                        id: fMouse; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (parent.canNav) webView.goForward()
                    }
                }

                Rectangle {
                    implicitWidth: 26; implicitHeight: 26; radius: 4
                    color: rMouse.containsMouse ? "#e2e8f0" : "transparent"
                    Label {
                        anchors.centerIn: parent; text: "↻"
                        font.pixelSize: 14; color: "#475569"
                    }
                    MouseArea {
                        id: rMouse; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: webView.reload()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 24; radius: 4
                    color: "#ffffff"
                    border.color: "#e2e8f0"; border.width: 1

                    TextInput {
                        id: urlInput
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        verticalAlignment: TextInput.AlignVCenter
                        text: webView.url.toString()
                        font.pixelSize: 11; color: "#334155"
                        selectByMouse: true
                        onAccepted: {
                            var url = text.trim();
                            if (url.length === 0) return;
                            if (url.indexOf("://") < 0) url = "https://" + url;
                            webView.url = url;
                        }
                    }
                }
            }
        }

        // ── WebEngineView ──
        WebEngineView {
            id: webView
            Layout.fillWidth: true
            Layout.fillHeight: true

            profile: WebEngineProfile {
                onDownloadRequested: function(download) {
                    if (root.downloadPath.length > 0) {
                        download.path = root.downloadPath + "/" + download.downloadFileName;
                        download.accept();
                        root.downloadRequested(download.url.toString(), download.downloadFileName);
                    } else {
                        download.cancel();
                    }
                }
            }

            onLoadingChanged: function(load) {
                if (load.status === WebEngineView.LoadSucceededStatus) {
                    urlInput.text = webView.url.toString();
                }
            }
        }
    }

    // ── Empty state overlay ──
    Rectangle {
        anchors.fill: parent
        visible: webView.url.toString() === "" || webView.url.toString() === "about:blank"
        color: "#f8fafc"
        z: 1

        Column {
            anchors.centerIn: parent; spacing: 10

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "内嵌浏览器"; color: "#94a3b8"
                font.pixelSize: 18; font.bold: true
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "在地址栏输入字幕网站URL后按 Enter 键访问"
                color: "#c7d2e0"; font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "点击下载的 .srt 字幕将自动保存到「字幕下载路径」"
                color: "#c7d2e0"; font.pixelSize: 12
            }
        }
    }
}
