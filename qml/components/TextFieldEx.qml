//TextFieldEx
import QtQuick
import QtQuick.Controls

TextField {
    id: root

    // ── 输入类型 ──
    // 0=文本, 1=整数, 2=小数
    property int editType: 0
    property double minNumber: -10000
    property double maxNumber: 10000

    // 失焦后保持高亮边框（指示当前值为有效值）
    property bool highlightOnValid: false

    readonly property var _intValidator: IntValidator {
        bottom: root.minNumber
        top: root.maxNumber
    }
    readonly property var _doubleValidator: DoubleValidator {
        bottom: root.minNumber
        top: root.maxNumber
        decimals: 4
        notation: DoubleValidator.StandardNotation
    }
    validator: editType === 1 ? _intValidator : editType === 2 ? _doubleValidator : null

    // 数字类型编辑时自动修正：
    // - 当前值 > maxNumber → 肯定无效 → 立刻钳位
    // - 当前值 < minNumber → 已有明确非零数字（如 0.02）→ 立刻钳位
    // → 只有零、点、负号（如 0 0. 0.0）→ 保留待继续输入
    onTextEdited: {
        if (editType === 0)
            return;
        var t = text.trim();
        if (t === "" || t === "-" || t === "." || t === "-.")
            return;
        var val = editType === 1 ? parseInt(t) : parseFloat(t);
        if (isNaN(val))
            return;
        var clamped = Math.max(Number(minNumber), Math.min(Number(maxNumber), val));
        if (clamped !== val) {
            // 上限超范围，或下限超范围且已有非零数字 → 钳位
            if (val > Number(maxNumber))
                text = editType === 1 ? Math.floor(clamped).toString() : clamped.toString();
            else if (val < Number(minNumber)) {
                var onlyZero = true;
                for (var i = 0; i < t.length; i++) {
                    var c = t.charAt(i);
                    if (c >= '1' && c <= '9') {
                        onlyZero = false;
                        break;
                    }
                }
                if (!onlyZero)
                    text = editType === 1 ? Math.floor(clamped).toString() : clamped.toString();
            }
        }
    }

    // 编辑完成（失焦/回车）时双向钳位
    Component.onCompleted: {
        if (editType !== 0) {
            root.editingFinished.connect(function () {
                var t = root.text.trim();
                if (t === "")
                    return;
                var val = editType === 1 ? parseInt(t) : parseFloat(t);
                if (!isNaN(val)) {
                    var clamped = Math.max(Number(minNumber), Math.min(Number(maxNumber), val));
                    if (clamped !== val)
                        root.text = editType === 1 ? Math.floor(clamped).toString() : clamped.toString();
                }
            });
        }
    }

    // ── 主题支持 ──
    property string paletteGroup: ""
    property var bgColor: undefined
    property var disabledBgColor: undefined
    property var textColor: undefined
    property var disabledTextColor: undefined
    property var phColor: undefined
    property var selColor: undefined
    property var selTextColor: undefined
    property var borderColor: undefined
    property var disabledBorderColor: undefined
    property var focusBorderColor: undefined
    property var focusRingColor: undefined
    property var cursorColor: undefined

    readonly property var _p: themeManager.palette
    readonly property color _bgColor: root.bgColor !== undefined ? root.bgColor : (paletteGroup ? (_p[paletteGroup + "_bgColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _disabledBgColor: root.disabledBgColor !== undefined ? root.disabledBgColor : (paletteGroup ? (_p[paletteGroup + "_disabledBgColor"] || "#F8FAFC") : "#F8FAFC")
    readonly property color _textColor: root.textColor !== undefined ? root.textColor : (paletteGroup ? (_p[paletteGroup + "_textColor"] || "#1E293B") : "#1E293B")
    readonly property color _disabledTextColor: root.disabledTextColor !== undefined ? root.disabledTextColor : (paletteGroup ? (_p[paletteGroup + "_disabledTextColor"] || "#94A3B8") : "#94A3B8")
    readonly property color _phColor: root.phColor !== undefined ? root.phColor : (paletteGroup ? (_p[paletteGroup + "_phColor"] || "#B0BEC5") : "#B0BEC5")
    readonly property color _selColor: root.selColor !== undefined ? root.selColor : (paletteGroup ? (_p[paletteGroup + "_selColor"] || "#3B82F6") : "#3B82F6")
    readonly property color _selTextColor: root.selTextColor !== undefined ? root.selTextColor : (paletteGroup ? (_p[paletteGroup + "_selTextColor"] || "#FFFFFF") : "#FFFFFF")
    readonly property color _borderColor: root.borderColor !== undefined ? root.borderColor : (paletteGroup ? (_p[paletteGroup + "_borderColor"] || "#BDBDBD") : "#BDBDBD")
    readonly property color _disabledBorderColor: root.disabledBorderColor !== undefined ? root.disabledBorderColor : (paletteGroup ? (_p[paletteGroup + "_disabledBorderColor"] || "#E2E8F0") : "#E2E8F0")
    readonly property color _focusBorderColor: root.focusBorderColor !== undefined ? root.focusBorderColor : (paletteGroup ? (_p[paletteGroup + "_focusBorderColor"] || "#3B82F6") : "#3B82F6")
    readonly property color _focusRingColor: root.focusRingColor !== undefined ? root.focusRingColor : (paletteGroup ? (_p[paletteGroup + "_focusRingColor"] || "#3B82F6") : "#3B82F6")
    readonly property color _cursorColor: root.cursorColor !== undefined ? root.cursorColor : (paletteGroup ? (_p[paletteGroup + "_cursorColor"] || "#3B82F6") : "#3B82F6")
    readonly property color _highlightBorderColor: root.focusBorderColor !== undefined ? root.focusBorderColor : (paletteGroup ? (_p[paletteGroup + "_highlightBorderColor"] || "#3B82F6") : "#3B82F6")

    implicitWidth: 76
    implicitHeight: 26

    color: enabled ? root._textColor : root._disabledTextColor
    selectionColor: root._selColor
    selectedTextColor: root._selTextColor
    placeholderTextColor: root._phColor
    font.pixelSize: 13
    verticalAlignment: Text.AlignVCenter
    leftPadding: 12
    rightPadding: 12
    topPadding: 0
    bottomPadding: 0

    background: Rectangle {
        radius: 6
        color: root.enabled ? root._bgColor : root._disabledBgColor
        border.width: root.activeFocus ? 1.5 : root.highlightOnValid ? 1.5 : 1
        border.color: root.activeFocus ? root._focusBorderColor : root.highlightOnValid ? root._highlightBorderColor : root.enabled ? root._borderColor : root._disabledBorderColor

        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 8
            color: "transparent"
            border.width: 0
            visible: root.activeFocus && root.enabled

            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                radius: 10
                color: root._focusRingColor
                opacity: 0.08
            }
        }
    }

    cursorDelegate: Rectangle {
        width: 1.5
        color: root._cursorColor
        visible: root.activeFocus
    }
}
