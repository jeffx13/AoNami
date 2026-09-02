import QtQuick
import ".."

Rectangle {
    id: row

    property bool current: false
    property bool hovered: false

    radius: 10
    color: current ? Theme.accentSoft : hovered ? Theme.overlayLine : "transparent"
    border.color: current ? Theme.accentMuted : "transparent"
    border.width: current ? 1 : 0
    Behavior on color { ColorAnimation { duration: 100 } }

    Rectangle {
        visible: row.current
        width: 3
        radius: 1.5
        color: Theme.accent
        anchors {
            left: parent.left; top: parent.top; bottom: parent.bottom
            leftMargin: 2; topMargin: 8; bottomMargin: 8
        }
    }
}
