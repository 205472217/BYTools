// MenuEx.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: root

    flags: Qt.Popup | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "#FFFFFF"

    property var items: []
    property Window transientParent: null
    property int submenuParentIndex: -1
    property int submenuDelay: 100 // 子菜单延迟显示时间(ms)
    
    signal itemTriggered(int index, string action)
    signal closed

    width: 200

    height: {
        var h = 8
        var arr = root.items
        for (var i = 0; i < arr.length; i++) {
            h += arr[i].type === "separator" ? 1 : 32
        }
        return h
    }

    // 子菜单实例
    property var activeSubmenu: null
    property int hoveredSubmenuIndex: -1
    property var hoveredSubmenuData: null
    property Timer submenuOpenTimer: Timer {
        interval: root.submenuDelay
        repeat: false
        onTriggered: {
            if (hoveredSubmenuIndex >= 0 && hoveredSubmenuData) {
                showSubmenu(hoveredSubmenuIndex, hoveredSubmenuData)
            }
        }
    }

    function popup(posX, posY) {
        if (root.transientParent)
            root.transientParent = root.transientParent
        var clampedX = Math.max(0, Math.min(posX, Screen.width - root.width))
        var clampedY = Math.max(0, Math.min(posY, Screen.height - root.height))
        root.x = clampedX
        root.y = clampedY
        show()
        raise()
        requestActivate()
    }

    function closeMenu() {
        closeAllSubmenus()
        hide()
    }

    function closeAllSubmenus() {
        if (activeSubmenu) {
            var sub = activeSubmenu
            activeSubmenu = null
            sub.closeMenu()
            sub.destroy()
        }
    }

    function showSubmenu(index, itemData) {
        // 如果已有子菜单且不是同一个，先关闭
        if (activeSubmenu) {
            if (activeSubmenu.submenuParentIndex === index) {
                return // 同一个子菜单，不重复打开
            }
            activeSubmenu.closeMenu()
            activeSubmenu.destroy()
            activeSubmenu = null
        }

        if (!itemData.submenu || itemData.submenu.length === 0) {
            return
        }

        // 计算子菜单位置
        var itemY = 8 + index * 32 // 8是顶部边距，32是每个菜单项高度
        var subX = root.x + root.width - 2
        var subY = root.y + itemY

        // 确保子菜单不超出屏幕
        var subWidth = 200
        if (subX + subWidth > Screen.width) {
            subX = root.x - subWidth + 2
        }
        if (subY + 200 > Screen.height) {
            subY = Screen.height - 200
        }
        if (subY < 0) subY = 0

        // 创建子菜单
        var component = Qt.createComponent("MenuEx.qml")
        if (component.status === Component.Ready) {
            var submenu = component.createObject(root, {
                items: itemData.submenu,
                transientParent: root,
                submenuParentIndex: index
            })
            
            if (submenu) {
                submenu.itemTriggered.connect(function(idx, action) {
                    root.itemTriggered(idx, action)
                    closeAllSubmenus()
                })
                
                submenu.closed.connect(function() {
                    if (activeSubmenu === submenu) {
                        activeSubmenu = null
                    }
                })
                
                submenu.x = subX
                submenu.y = subY
                submenu.show()
                submenu.raise()
                submenu.requestActivate()
                activeSubmenu = submenu
            }
        }
    }

    onVisibleChanged: {
        if (!visible) {
            closeAllSubmenus()
            closed()
        }
    }

    onActiveChanged: {
        if (!active && (!activeSubmenu || !activeSubmenu.active)) {
            closeAllSubmenus()
            hide()
        }
    }

    // 点击外部关闭菜单
    onActiveFocusItemChanged: {
        if (!activeFocusItem && active && !activeSubmenu) {
            closeMenu()
        }
    }

    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Escape) {
                if (activeSubmenu) {
                    closeAllSubmenus()
                } else {
                    root.closeMenu()
                }
                event.accepted = true
            }
            if (event.key === Qt.Key_Right && activeSubmenu) {
                // 右箭头聚焦到子菜单
                if (activeSubmenu) {
                    activeSubmenu.requestActivate()
                    event.accepted = true
                }
            }
            if (event.key === Qt.Key_Left) {
                closeAllSubmenus()
                event.accepted = true
            }
        }
    }

    Column {
        id: column
        anchors.fill: parent
        anchors.margins: 4
        spacing: 0
        clip: true

        Repeater {
            model: root.items

            delegate: Item {
                id: delegateItem
                width: parent ? parent.width : 200
                height: modelData.type === "separator" ? 1 : 32

                property bool hasSubmenu: modelData.submenu !== undefined

                Rectangle {
                    anchors.fill: parent
                    color: "#D0D0D0"
                    visible: modelData.type === "separator"
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 4
                    color: {
                        if (modelData.type === "separator") return "transparent"
                        if (modelData.enabled === false) return "transparent"
                        if (mouseArea.containsMouse) return "#E8E8E8"
                        return "transparent"
                    }
                }

                RowLayout {
                    visible: modelData.type !== "separator"
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Image {
                        width: 16
                        height: 16
                        sourceSize.width: 16
                        sourceSize.height: 16
                        source: modelData.icon || ""
                        visible: modelData.icon !== undefined && modelData.icon !== ""
                        opacity: modelData.enabled !== false ? 1.0 : 0.4
                    }

                    Label {
                        text: modelData.text || ""
                        Layout.fillWidth: true
                        color: modelData.enabled !== false ? "#000000" : "#AAAAAA"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    Label {
                        text: modelData.checked ? "✓" : ""
                        visible: modelData.checkable === true
                        color: "#000000"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        text: "▶"
                        visible: hasSubmenu
                        color: modelData.enabled !== false ? "#666666" : "#AAAAAA"
                        font.pixelSize: 10
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: modelData.type !== "separator" && modelData.enabled !== false
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    
                    onEntered: {
                        root.hoveredSubmenuIndex = index
                        root.hoveredSubmenuData = modelData
                        if (hasSubmenu) {
                            root.submenuOpenTimer.restart()
                        } else {
                            root.submenuOpenTimer.stop()
                            if (root.activeSubmenu) {
                                root.closeAllSubmenus()
                            }
                        }
                    }

                    onExited: {
                        root.submenuOpenTimer.stop()
                    }

                    onClicked: {
                        if (hasSubmenu) {
                            if (root.activeSubmenu && root.activeSubmenu.submenuParentIndex === index) {
                                root.closeAllSubmenus()
                            } else {
                                root.showSubmenu(index, modelData)
                            }
                        } else {
                            root.itemTriggered(index, modelData.action || "")
                            root.closeMenu()
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 1
        border.color: "#C0C0C0"
        z: -1
    }

    // 清理资源
    Component.onDestruction: {
        closeAllSubmenus()
    }
}