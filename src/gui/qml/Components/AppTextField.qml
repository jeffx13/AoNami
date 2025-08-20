import QtQuick.Controls
import QtQuick
import Qt5Compat.GraphicalEffects

TextField {
    id: textField
    property color checkedColor: "#2B2F44"
    property color textColor: "#E5E7EB"
    property color accentColor: "#4E5BF2"
    property color surfaceColor: "#0F172A"
    property int fontSize: 20
    readonly property int scaledFontSize: fontSize * (root.maximised ? fontScaleFactor : 1)
    property int fontScaleFactor: root.fontSizeMultiplier

    signal doubleClicked(var event)

    font.family: "QTxiaotu"

    font.pixelSize: scaledFontSize
    font.weight: Font.Thin
    color: textField.textColor

    antialiasing: true
    hoverEnabled: true

    background: Rectangle {
        radius: 12
        color: surfaceColor
        border.color: textField.activeFocus ? textField.accentColor : (textField.hovered ? Qt.lighter(textField.accentColor, 1.3) : textField.checkedColor)
        border.width: 1
        opacity: textField.enabled ? 1 : 0.7

        Behavior on border.color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

        layer.enabled: textField.hovered || textField.activeFocus
        layer.effect: DropShadow {
            transparentBorder: true
            color: Qt.rgba(78/255, 91/255, 242/255, textField.activeFocus ? 0.6 : 0.35)
            samples: textField.activeFocus ? 18 : 12
            horizontalOffset: 0
            verticalOffset: textField.activeFocus ? 6 : 3
        }
    }
    leftPadding: 12
    rightPadding: 12

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        acceptedButtons: Qt.NoButton
    }

    onDoubleClicked: selectAll()
}
