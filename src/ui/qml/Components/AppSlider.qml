import QtQuick
import QtQuick.Controls
import ".."

Slider {
    id: slider

    property color accentColor: Theme.accent
    property int   decimals: 0
    property string unitSuffix: ""

    implicitHeight: 30
    implicitWidth: 140
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    live: true

    readonly property bool active: hovered || pressed

    background: Rectangle {
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        width: slider.availableWidth
        height: slider.active ? 8 : 5
        radius: height / 2
        color: Theme.border
        Behavior on height { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }

        Rectangle {
            width: slider.visualPosition * parent.width
            height: parent.height
            radius: parent.radius
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.accentLight }
                GradientStop { position: 1.0; color: slider.accentColor }
            }
        }
    }

    handle: Rectangle {
        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        width: 20; height: 20; radius: 10
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.accentLight }
            GradientStop { position: 1.0; color: slider.accentColor }
        }
        border.color: "#ffffff"
        border.width: 2
        scale: slider.pressed ? 1.22 : (slider.hovered ? 1.1 : 1.0)
        Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutBack } }

        Rectangle {
            anchors.centerIn: parent
            width: 6; height: 6; radius: 3
            color: "#ffffff"
        }

        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 14; height: width; radius: width / 2
            color: "transparent"
            border.color: slider.accentColor
            border.width: 2
            opacity: slider.active ? 0.4 : 0
            Behavior on opacity { NumberAnimation { duration: 130 } }
        }

        Rectangle {
            visible: slider.active
            anchors.horizontalCenter: parent.horizontalCenter
            y: -height - 8
            width: bubbleText.implicitWidth + 16
            height: 24
            radius: 7
            color: slider.accentColor

            Text {
                id: bubbleText
                anchors.centerIn: parent
                text: (slider.decimals > 0 ? Number(slider.value).toFixed(slider.decimals)
                                           : Math.round(slider.value)) + slider.unitSuffix
                color: "white"
                font.pixelSize: Globals.sp(16)
                font.weight: Font.Medium
            }

            Rectangle {
                anchors { top: parent.bottom; horizontalCenter: parent.horizontalCenter; topMargin: -3 }
                width: 8; height: 8; rotation: 45
                color: slider.accentColor
            }
        }
    }
}
