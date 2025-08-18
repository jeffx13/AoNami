import QtQuick.Controls
import QtQuick

Button {
    id: button
    property color backgroundDefaultColor: "#4E5BF2"
    property color backgroundPressedColor: Qt.darker(backgroundDefaultColor, 1.2)
    property color contentItemTextColor: "white"
    property int fontSize: 20
    readonly property int scaledFontSize: fontSize * (root.maximised ? fontScaleFactor : 1)
    property int fontScaleFactor: root.fontSizeMultiplier
    property alias radius: backRect.radius

    hoverEnabled: true
    scale: down ? 0.98 : (hovered ? 1.03 : 1.0)
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

    contentItem: Text {
        text: button.text
        color: button.contentItemTextColor
        font.weight: Font.Thin
        font.pixelSize: button.scaledFontSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        wrapMode: Text.Wrap
    }

    focusPolicy: Qt.NoFocus
    background: Rectangle {
        id: backRect
        color: button.down ? button.backgroundPressedColor : (button.hovered ? Qt.lighter(button.backgroundDefaultColor, 1.1) : button.backgroundDefaultColor)
        radius: 10
        border.color: button.hovered ? Qt.lighter(button.backgroundDefaultColor, 1.25) : "transparent"
        border.width: button.hovered ? 1 : 0
    }
}
