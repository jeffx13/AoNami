import QtQuick
import QtQuick.Controls
import ".."

SpinBox {
    id: spin

    property color accentColor: Theme.accent
    property color surfaceColor: Theme.surface
    property color borderColor: Theme.border

    implicitHeight: 38
    implicitWidth: 110
    hoverEnabled: true
    editable: true
    inputMethodHints: Qt.ImhDigitsOnly
    leftPadding: 34
    rightPadding: 34

    contentItem: TextInput {
        text: spin.textFromValue(spin.value, spin.locale)
        readOnly: !spin.editable
        selectByMouse: true
        validator: spin.validator
        inputMethodHints: Qt.ImhDigitsOnly
        font.pixelSize: Globals.sp(18)
        font.weight: Font.Medium
        color: Theme.textPrimary
        selectionColor: spin.accentColor
        selectedTextColor: "#ffffff"
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    down.indicator: Rectangle {
        implicitWidth: 30
        implicitHeight: parent.height
        anchors.left: parent.left
        radius: 10
        color: spin.down.pressed ? accentColor
             : spin.down.hovered ? Theme.border : "transparent"
        Behavior on color { ColorAnimation { duration: 80 } }

        AppIcon {
            anchors.centerIn: parent
            name: "minus"
            size: 15
            color: spin.down.pressed ? "white" : Theme.textSecondary
        }
    }

    up.indicator: Rectangle {
        implicitWidth: 30
        implicitHeight: parent.height
        anchors.right: parent.right
        radius: 10
        color: spin.up.pressed ? accentColor
             : spin.up.hovered ? Theme.border : "transparent"
        Behavior on color { ColorAnimation { duration: 80 } }

        AppIcon {
            anchors.centerIn: parent
            name: "plus"
            size: 15
            color: spin.up.pressed ? "white" : Theme.textSecondary
        }
    }

    background: Rectangle {
        radius: 10
        color: spin.activeFocus ? Theme.surfaceAlt : surfaceColor
        border.color: spin.activeFocus ? accentColor
                    : spin.hovered     ? Theme.textMuted : borderColor
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }
}
