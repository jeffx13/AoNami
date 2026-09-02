import QtQuick
import QtQuick.Controls
import ".."

Switch {
    id: sw

    implicitWidth: 48
    implicitHeight: 27
    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    indicator: Rectangle {
        implicitWidth: sw.implicitWidth
        implicitHeight: sw.implicitHeight
        x: sw.leftPadding
        y: parent.height / 2 - height / 2
        radius: height / 2
        color: Theme.border
        border.width: 1
        border.color: sw.checked ? Qt.lighter(Theme.accent, 1.2)
                    : sw.hovered ? Theme.textMuted : Theme.border
        Behavior on border.color { ColorAnimation { duration: 150 } }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            opacity: sw.checked ? 1 : 0
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.accentLight }
                GradientStop { position: 1.0; color: Theme.accent }
            }
            Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

            Rectangle {
                anchors { left: parent.left; right: parent.right; top: parent.top
                          leftMargin: 8; rightMargin: 8; topMargin: 1 }
                height: 1
                radius: 1
                color: Qt.rgba(1, 1, 1, 0.25)   // gloss on the accent-filled track, not page chrome
            }
        }

        Rectangle {
            width: parent.height - 6
            height: width
            radius: width / 2
            y: 3
            x: sw.checked ? parent.width - width - 3 : 3
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#ffffff" }
                GradientStop { position: 1.0; color: sw.checked ? "#EAF0FF" : "#D7DCE6" }
            }
            border.width: 1
            border.color: sw.checked ? Qt.lighter(Theme.accent, 1.3) : "#AAB2C2"

            Behavior on x { NumberAnimation { duration: 190; easing.type: Easing.OutBack; easing.overshoot: 1.15 } }

            scale: sw.down ? 0.9 : (sw.hovered ? 1.05 : 1.0)
            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

            Rectangle {
                anchors.centerIn: parent
                width: 4; height: 4; radius: 2
                color: sw.checked ? Theme.accent : Theme.textDisabled
                opacity: 0.55
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }
    }
}
