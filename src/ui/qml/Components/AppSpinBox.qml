import QtQuick
import QtQuick.Controls
import ".."

SpinBox {
    id: spin

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
        selectionColor: Theme.accent
        selectedTextColor: Theme.onColor(Theme.accent)
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    down.indicator: Rectangle {
        implicitWidth: 30
        implicitHeight: parent.height
        anchors.left: parent.left
        radius: 10
        color: spin.down.pressed ? Theme.accent
             : spin.down.hovered ? Theme.border : "transparent"
        Behavior on color { ColorAnimation { duration: 80 } }

        AppIcon {
            anchors.centerIn: parent
            name: "minus"
            size: 15
            color: spin.down.pressed ? Theme.onColor(Theme.accent) : Theme.textSecondary
        }
    }

    up.indicator: Rectangle {
        implicitWidth: 30
        implicitHeight: parent.height
        anchors.right: parent.right
        radius: 10
        color: spin.up.pressed ? Theme.accent
             : spin.up.hovered ? Theme.border : "transparent"
        Behavior on color { ColorAnimation { duration: 80 } }

        AppIcon {
            anchors.centerIn: parent
            name: "plus"
            size: 15
            color: spin.up.pressed ? Theme.onColor(Theme.accent) : Theme.textSecondary
        }
    }

    background: Rectangle {
        radius: 10
        color: spin.activeFocus ? Theme.surfaceAlt : Theme.surface
        border.color: spin.activeFocus ? Theme.accent
                    : spin.hovered     ? Theme.textMuted : Theme.border
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }
}
