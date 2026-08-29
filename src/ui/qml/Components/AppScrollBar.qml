import QtQuick
import QtQuick.Controls
import ".."

ScrollBar {
    id: bar

    property color barColor: Theme.textMuted
    property real  barOpacity: 0.6
    property bool  showTrack: false

    policy: ScrollBar.AsNeeded
    width: 8

    contentItem: Rectangle {
        radius: width / 2
        color: bar.barColor
        opacity: bar.pressed ? 1.0 : (bar.hovered ? Math.min(1.0, bar.barOpacity + 0.25) : bar.barOpacity)
        Behavior on opacity { NumberAnimation { duration: 120 } }
    }

    background: Rectangle {
        visible: bar.showTrack
        radius: width / 2
        color: Theme.surfaceAlt
    }
}
