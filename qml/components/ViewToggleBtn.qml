//ViewToggleBtn
import QtQuick
import QtQuick.Controls

Item {
    id: vtb
    width: 24
    height: 24

    /// 外部可绑定属性
    // 图标类型：grid宫格 / list列表 / sort排序 / filter筛选
    property string iconType: ""
    // 激活高亮状态（筛选有条件时置true）
    property bool isActive: false
    // 仅sort类型生效：false升序、true降序
    property bool sortDesc: false
    // 鼠标悬浮提示文本
    property string tip: ""

    // ── 主题支持 ──
    property string paletteGroup: ""
    property color activeColor: "#26c6da"
    property color hoverColor: "#ffffff"
    property color normalColor: "#cccccc"
    property color dimColor: "#5a6470"

    readonly property var _p: themeManager.palette
    readonly property color _activeColor:
        paletteGroup ? (_p[paletteGroup + "_activeColor"] || activeColor) : activeColor
    readonly property color _hoverColor:
        paletteGroup ? (_p[paletteGroup + "_hoverColor"] || hoverColor) : hoverColor
    readonly property color _normalColor:
        paletteGroup ? (_p[paletteGroup + "_normalColor"] || normalColor) : normalColor
    readonly property color _dimColor:
        paletteGroup ? (_p[paletteGroup + "_dimColor"] || dimColor) : dimColor

    /// 点击信号，外部onClicked接收
    signal clicked()

    Canvas {
        id: iconCanvas
        anchors.centerIn: parent
        width: 16
        height: 16
        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const mainColor = vtb.isActive ? vtb._activeColor : (ma.containsMouse ? vtb._hoverColor : vtb._normalColor)
            const dimColor = vtb._dimColor
            ctx.strokeStyle = mainColor
            ctx.fillStyle = mainColor
            ctx.lineWidth = 1.8
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            const w = width
            const h = height

            if (vtb.iconType === "grid") {
                const margin = 1.5
                const blockSize = (w - margin * 3) / 2
                ctx.fillRect(margin, margin, blockSize, blockSize)
                ctx.fillRect(margin * 2 + blockSize, margin, blockSize, blockSize)
                ctx.fillRect(margin, margin * 2 + blockSize, blockSize, blockSize)
                ctx.fillRect(margin * 2 + blockSize, margin * 2 + blockSize, blockSize, blockSize)
            } else if (vtb.iconType === "list") {
                for (let i = 0; i < 3; i++) {
                    const y = 4 + i * 4
                    ctx.beginPath()
                    ctx.arc(3, y, 1.5, 0, Math.PI * 2)
                    ctx.fill()

                    ctx.beginPath()
                    ctx.moveTo(6, y)
                    ctx.lineTo(15, y)
                    ctx.stroke()
                }
            } else if (vtb.iconType === "sort") {
                ctx.fillStyle = vtb.sortDesc ? dimColor : mainColor
                ctx.beginPath()
                ctx.moveTo(8, 2)
                ctx.lineTo(4, 7)
                ctx.lineTo(12, 7)
                ctx.closePath()
                ctx.fill()

                ctx.fillStyle = vtb.sortDesc ? mainColor : dimColor
                ctx.beginPath()
                ctx.moveTo(8, 14)
                ctx.lineTo(4, 9)
                ctx.lineTo(12, 9)
                ctx.closePath()
                ctx.fill()
            } else if (vtb.iconType === "filter") {
                ctx.beginPath()
                ctx.moveTo(2, 2)
                ctx.lineTo(w - 2, 2)
                ctx.lineTo(w / 2 + 2, h / 2)
                ctx.lineTo(w / 2 + 2, h - 2)
                ctx.lineTo(w / 2 - 2, h - 2)
                ctx.lineTo(w / 2 - 2, h / 2)
                ctx.closePath()

                if (vtb.isActive) ctx.fill()
                else ctx.stroke()
            }
        }

        Connections {
            target: ma
            function onContainsMouseChanged() { iconCanvas.requestPaint() }
        }
        Connections {
            target: vtb
            function onIsActiveChanged() { iconCanvas.requestPaint() }
            function onSortDescChanged() { iconCanvas.requestPaint() }
            function onIconTypeChanged() { iconCanvas.requestPaint() }
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: vtb.clicked()
    }

    ToolTip {
        visible: ma.containsMouse && vtb.tip !== ""
        text: vtb.tip
        delay: 400
    }
}
