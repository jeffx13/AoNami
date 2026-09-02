import QtQuick
import QtQuick.Controls
import ".."

ScrollBar {
    id: bar

    property color barColor: Theme.accent
    property real  barOpacity: 0.45
    property bool  showTrack: true

    // Set to a view to sit just outside its right edge instead of overlaying its content.
    property Item attachTo: null

    parent: attachTo ? attachTo.parent : undefined
    anchors.top: attachTo ? attachTo.top : undefined
    anchors.left: attachTo ? attachTo.right : undefined
    anchors.bottom: attachTo ? attachTo.bottom : undefined

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
