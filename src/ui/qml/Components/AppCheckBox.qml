import QtQuick
import QtQuick.Controls
import ".."

CheckBox {
    id: control

    readonly property int boxSize: 22

    implicitWidth: boxSize + leftPadding + rightPadding
    implicitHeight: boxSize + 8
    leftPadding: 0
    rightPadding: 0
    spacing: 0
    hoverEnabled: true

    indicator: Rectangle {
        implicitWidth: control.boxSize
        implicitHeight: control.boxSize
        anchors.centerIn: parent
        radius: 6
        color: control.checked ? Theme.accent : Theme.surface
        border.color: control.checked ? Qt.lighter(Theme.accent, 1.2)
                    : control.hovered ? Theme.textMuted : Theme.border
        border.width: 1

        Behavior on color { ColorAnimation { duration: 140 } }
        Behavior on border.color { ColorAnimation { duration: 140 } }

        AppIcon {
            anchors.centerIn: parent
            name: "check"
            size: Math.round(parent.height * 0.62)
            color: Theme.onColor(Theme.accent)
            visible: control.checked
            opacity: control.checked ? 1.0 : 0.0
            scale: control.checked ? 1.0 : 0.5
            Behavior on opacity { NumberAnimation { duration: 120 } }
            Behavior on scale { NumberAnimation { duration: 160; easing.type: Easing.OutBack } }
        }
    }

    contentItem: Item { implicitWidth: 0; implicitHeight: 0 }
}
