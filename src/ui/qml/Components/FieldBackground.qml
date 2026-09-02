import QtQuick
import ".."

Rectangle {
    id: bg

    property bool focused: false
    property bool hovered: false
    property color restingBorder: Theme.border

    radius: 10
    color: focused ? Theme.surfaceAlt : Theme.surface
    border.color: focused ? Theme.accent
                : hovered ? Qt.lighter(Theme.accent, 1.3)
                          : restingBorder
    border.width: 1
    Behavior on border.color { ColorAnimation { duration: 150 } }
    Behavior on color { ColorAnimation { duration: 150 } }

    Rectangle {
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: -1 }
        height: 2
        radius: 1
        width: bg.focused ? parent.width - 20 : 0
        color: Theme.accent
        opacity: bg.focused ? 1.0 : 0.0
        Behavior on width { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }
}
