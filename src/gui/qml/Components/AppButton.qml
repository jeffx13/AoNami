import QtQuick.Controls
import QtQuick
import Qt5Compat.GraphicalEffects

Button {
    id: button
    property color backgroundDefaultColor: "#4E5BF2"
    property color backgroundPressedColor: Qt.darker(backgroundDefaultColor, 1.2)
    property color backgroundHoverColor: Qt.lighter(backgroundDefaultColor, 1.1)
    property color contentItemTextColor: "white"
    property color borderColor: "#ffffff22"
    property color hoverBorderColor: Qt.lighter(backgroundDefaultColor, 1.4)
    property real cornerRadius: 12
    property int fontSize: 20
    readonly property int scaledFontSize: fontSize * (root.maximised ? fontScaleFactor : 1)
    property int fontScaleFactor: root.fontSizeMultiplier
    property alias radius: backRect.radius

    hoverEnabled: true
    scale: down ? 0.985 : (hovered ? 1.03 : 1.0)
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

    contentItem: Text {
        text: button.text
        color: button.contentItemTextColor
        font.weight: Font.Thin
        font.pixelSize: button.scaledFontSize
        font.letterSpacing: 0.5
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        wrapMode: Text.Wrap
    }

    focusPolicy: Qt.NoFocus
    background: Rectangle {
        id: backRect
        color: button.down ? button.backgroundPressedColor : (button.hovered ? button.backgroundHoverColor : button.backgroundDefaultColor)
        radius: cornerRadius
        border.color: button.hovered ? hoverBorderColor : borderColor
        border.width: button.hovered ? 1 : 0

        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Behavior on border.width { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

        layer.enabled: button.hovered || button.down
        layer.effect: DropShadow {
            transparentBorder: true
            color: Qt.rgba(78/255, 91/255, 242/255, button.down ? 0.7 : 0.45)
            samples: button.down ? 20 : 14
            horizontalOffset: 0
            verticalOffset: button.down ? 6 : 3
        }
    }
}
