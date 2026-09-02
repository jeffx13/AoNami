import QtQuick
import ".."

// `period` is one full in-and-out cycle, not one leg of it.
Rectangle {
    id: ring

    property color ringColor: Theme.accent
    property real  peakOpacity: 0.4
    property real  peakScale: 1.3
    property int   period: 2000
    property bool  running: true

    color: "transparent"
    border.color: ringColor
    border.width: 1
    opacity: 0

    SequentialAnimation on opacity {
        loops: Animation.Infinite
        running: ring.running
        NumberAnimation { to: ring.peakOpacity; duration: ring.period / 2; easing.type: Easing.OutCubic }
        NumberAnimation { to: 0.0;              duration: ring.period / 2; easing.type: Easing.InCubic }
    }
    NumberAnimation on scale {
        loops: Animation.Infinite
        running: ring.running
        from: 1.0
        to: ring.peakScale
        duration: ring.period
    }
}
