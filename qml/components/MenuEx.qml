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
    property var activeParent: null
    property var activeSubmenu: null
    property int hoveredSubmenuIndex: -1
    property var hoveredSubmenuData: null

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
        var itemY = 4 + Math.max(index-1, 0) * 32 // 4是顶部边距，32是每个菜单项高度
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
                submenuParentIndex: index,
                activeParent: root
            })
            
            if (submenu) {
                submenu.itemTriggered.connect(function(idx, action) {
                    root.itemTriggered(idx, action)
                    closeRootMenu()
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

    function closeMenu() {
        closeSubmenu()
        hide()
    }

    function closeSubmenu() {
        if (activeSubmenu) {
            var sub = activeSubmenu
            activeSubmenu = null
            sub.closeMenu()
            sub.destroy()
        }
    }

    function closeRootMenu() {
        var top = root
        while (top.activeParent) {
            top = top.activeParent
        }

        function closeDescendants(n) {
            if (n.activeSubmenu) {
                var child = n.activeSubmenu
                n.activeSubmenu = null
                closeDescendants(child)
                child.closeMenu()
                child.destroy()
            }
        }

        closeDescendants(top)
        top.closeMenu()
    }

    onVisibleChanged: {
        if (!visible) {
            closeRootMenu()
        }
    }

    // 菜单窗口失去焦点
    onActiveChanged: {
        if (!active && (!activeSubmenu || !activeSubmenu.active)) {
            closeRootMenu()
        }
    }

    // 菜单窗口之外的区域获取焦点
    onActiveFocusItemChanged: {
        if (!activeFocusItem && active) {
            if (!activeSubmenu) {
                closeRootMenu()
            }
        }
    }

    // 组件销毁时关闭所有子菜单
    Component.onDestruction: {
        closeSubmenu()
    }

    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Escape) {
                root.closeRootMenu()
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
                            root.showSubmenu(index, modelData)
                        } else {
                            // 仅关闭子菜单，不影响父菜单
                            root.closeSubmenu()
                        }
                    }

                    onExited: {
                    }

                    onClicked: {
                        if (hasSubmenu) {
                            if (root.activeSubmenu && root.activeSubmenu.submenuParentIndex === index) {
                                root.closeSubmenu()
                            } else {
                                root.showSubmenu(index, modelData)
                            }
                        } else {
                            root.itemTriggered(index, modelData.action || "")
                            root.closeRootMenu()
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
}