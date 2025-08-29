import QtQuick
import QtQuick.Controls
import ".."
import Qt5Compat.GraphicalEffects

ComboBox {
    id: comboBox

    property color checkedColor: "#4E5BF2"
    property color surfaceColor: "#0F172A"
    property color borderColor: "#2B2F44"
    property color hoverBorderColor: Qt.lighter(checkedColor, 1.4)
    property color textColor: "#E5E7EB"
    property color currentIndexColor: "#111827"
    property int fontSize: 20
    readonly property int scaledFontSize: Math.round(fontSize * (Globals.maximised ? fontScaleFactor : 1))
    property int fontScaleFactor: Globals.fontSizeMultiplier

    // Name of the role used for display in the model (compatible with existing usage)
    // If empty, the control will auto-use modelData
    property string text: "text"
    property int hAlignment: Text.AlignHCenter
    property int vAlignment: Text.AlignVCenter

    // Placeholder shown when currentIndex < 0
    property string placeholderText: ""

    // Bind to the built-in textRole when provided
    textRole: comboBox.text && comboBox.text.length > 0 ? comboBox.text : ""

    delegate: ItemDelegate {
        width: comboBox.width
        height: comboBox.height
        enabled: !model.disabled
        opacity: enabled ? 1.0 : 0.5
        contentItem: Text {
            text: (comboBox.text.length === 0 || typeof model[comboBox.text] === "undefined") ? modelData : model[comboBox.text]
            color: comboBox.highlightedIndex === index ? "white" : comboBox.textColor
            elide: Text.ElideRight
            font.pixelSize: scaledFontSize
            verticalAlignment: vAlignment
            horizontalAlignment: hAlignment
        }

        background: Rectangle {
            width: parent.width
            height: parent.height
            color: {
                if (index === comboBox.currentIndex) return currentIndexColor
                return comboBox.highlightedIndex === index ? comboBox.checkedColor : comboBox.surfaceColor
            }
            radius: 12
        }
    }

    indicator: Canvas {
        id: canvas
        x: comboBox.width - width - 10
        y: (comboBox.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: comboBox
            function onPressedChanged(){
                canvas.requestPaint()
            }
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = comboBox.down ? "#BFC7FF" : comboBox.textColor
            context.fill();
        }
    }

    contentItem: Text {
        width: comboBox.background.width - comboBox.indicator.width - 10
        height: comboBox.background.height
        anchors.verticalCenter: parent.verticalCenter
        x: 10
        text: comboBox.currentIndex < 0 && comboBox.placeholderText.length > 0 ? comboBox.placeholderText : comboBox.displayText
        elide: Text.ElideRight
        horizontalAlignment: hAlignment
        verticalAlignment: vAlignment
        font.pixelSize: scaledFontSize
        font.weight: Font.Thin
        color: comboBox.down ? Qt.rgba(229, 231, 235, 0.75) : (comboBox.currentIndex < 0 && comboBox.placeholderText.length > 0 ? Qt.rgba(229, 231, 235, 0.55) : comboBox.textColor)
    }

    background: Rectangle {
        implicitWidth: 102
        implicitHeight: 41
        color: comboBox.surfaceColor
        radius: 12
        border.color: comboBox.activeFocus ? comboBox.checkedColor : (comboBox.hovered ? hoverBorderColor : borderColor)
        border.width: 1

        Behavior on border.color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

        layer.enabled: comboBox.hovered || comboBox.activeFocus
        layer.effect: DropShadow {
            transparentBorder: true
            color: Qt.rgba(78/255, 91/255, 242/255, comboBox.activeFocus ? 0.6 : 0.35)
            samples: comboBox.activeFocus ? 18 : 12
            horizontalOffset: 0
            verticalOffset: comboBox.activeFocus ? 6 : 3
        }
    }
    
    popup: Popup {
        parent: comboBox
        x: 0
        y: comboBox.height - 1
        transformOrigin: Item.TopLeft
        width: comboBox.width
        implicitHeight: contentItem.implicitHeight
        padding: 0
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        contentItem: ListView {
            implicitHeight: contentHeight
            model: comboBox.popup.visible ? comboBox.delegateModel : null
            clip: true
            currentIndex: comboBox.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }
        background: Rectangle {
            color: comboBox.surfaceColor
            border.color: comboBox.hovered ? hoverBorderColor : borderColor
            border.width: 1
            radius: 12
            clip: true
            layer.enabled: true
            layer.effect: DropShadow {
                transparentBorder: true
                color: Qt.rgba(0, 0, 0, 0.5)
                samples: 18
                horizontalOffset: 0
                verticalOffset: 6
            }
        }
    }
}
