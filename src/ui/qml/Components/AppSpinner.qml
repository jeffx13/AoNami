pragma ComponentBehavior: Bound
import QtQuick
import ".."

Item {
    id: spinner

    property color color: Theme.accent
    property int dotCount: 8
    property int dotSize: 6
    property real radius: 14
    property int duration: 1200
    property bool running: true

    implicitWidth: radius * 2 + dotSize
    implicitHeight: radius * 2 + dotSize

    Item {
        id: rotator
        anchors.centerIn: parent
        width: parent.width
        height: parent.height

        NumberAnimation on rotation {
            from: 0
            to: 360
            duration: spinner.duration
            loops: Animation.Infinite
            running: spinner.running
        }

        Repeater {
            model: spinner.dotCount
            delegate: Rectangle {
                id: dot
                required property int index
                readonly property real angle: index * (360 / spinner.dotCount) * (Math.PI / 180)

                x: rotator.width / 2 + spinner.radius * Math.cos(angle) - width / 2
                y: rotator.height / 2 + spinner.radius * Math.sin(angle) - height / 2
                width: spinner.dotSize * (1.0 - index * 0.06)
                height: width
                radius: width / 2
                color: spinner.color
                opacity: 1.0 - (index / spinner.dotCount) * 0.85

                Rectangle {
                    visible: dot.index === 0
                    anchors.centerIn: parent
                    width: parent.width + 8
                    height: parent.height + 8
                    radius: height / 2
                    color: "transparent"
                    border.color: spinner.color
                    border.width: 1
                    opacity: 0.3
                }
            }
        }
    }
}