import QtQuick
import QtQuick.Controls
import ".."
import Qt5Compat.GraphicalEffects

Switch {
    id: control

    // Theme
    property color accentColor: "#4E5BF2"
    property color trackOnColor: "#1A2660"
    property color trackOffColor: "#0F172A"
    property color borderColor: "#1F2937"
    property color hoverBorderColor: Qt.lighter(accentColor, 1.4)
    property color knobColor: "#E5E7EB"
    // Brighter on gradient
    property color onBrightStart: Qt.lighter(accentColor, 1.45)
    property color onBrightEnd: accentColor

    // Typography / scaling (for any optional label outside)
    property int fontSize: 20
    readonly property int scaledFontSize: Math.round(fontSize * (Globals.maximised ? fontScaleFactor : 1))
    property int fontScaleFactor: Globals.fontSizeMultiplier

    // Sizing
    property int switchWidth: 52
    property int switchHeight: 28
    implicitWidth: switchWidth
    implicitHeight: switchHeight

    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    indicator: Rectangle {
        id: track
        implicitWidth: control.switchWidth
        implicitHeight: control.switchHeight
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: height / 2
        color: control.checked ? control.trackOnColor : control.trackOffColor
        border.color: control.checked ? control.accentColor : (control.hovered ? control.hoverBorderColor : control.borderColor)
        border.width: 1

        Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }

        layer.enabled: control.hovered || control.activeFocus
        layer.effect: DropShadow {
            transparentBorder: true
            color: Qt.rgba(78/255, 91/255, 242/255, control.activeFocus ? 0.55 : 0.35)
            samples: control.activeFocus ? 18 : 12
            horizontalOffset: 0
            verticalOffset: control.activeFocus ? 6 : 3
        }

        // Bright gradient overlay when ON
        Rectangle {
            id: onFill
            anchors.fill: parent
            radius: parent.radius
            opacity: control.checked ? 1.0 : 0.0
            gradient: Gradient {
                GradientStop { position: 0.0; color: control.onBrightStart }
                GradientStop { position: 1.0; color: control.onBrightEnd }
            }
            Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
        }

        // Quick shine sweep on toggle ON
        Rectangle {
            id: shine
            anchors.verticalCenter: parent.verticalCenter
            x: -width
            width: Math.max(parent.height, parent.width * 0.42)
            height: parent.height - 8
            radius: height / 2
            opacity: 0.0
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(255,255,255,0.0) }
                GradientStop { position: 0.5; color: Qt.rgba(255,255,255,0.35) }
                GradientStop { position: 1.0; color: Qt.rgba(255,255,255,0.0) }
            }
        }

        // Knob
        Rectangle {
            id: knob
            width: parent.height - 6
            height: parent.height - 6
            radius: height / 2
            y: 3
            x: control.checked ? parent.width - width - 3 : 3
            color: control.enabled ? control.knobColor : Qt.lighter(control.knobColor, 1.4)
            border.color: control.checked ? control.accentColor : control.borderColor
            border.width: 1
            scale: 1.0

            Behavior on x { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
            Behavior on border.color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutBack } }

            layer.enabled: true
            layer.effect: DropShadow {
                transparentBorder: true
                color: Qt.rgba(0, 0, 0, 0.35)
                samples: control.checked ? 18 : 14
                horizontalOffset: 0
                verticalOffset: control.checked ? 4 : 3
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                border.color: "#00000022"
                border.width: 1
            }

            // Knob inner gloss
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.7
                height: parent.height * 0.5
                radius: height / 2
                color: Qt.rgba(255,255,255, control.checked ? 0.22 : 0.14)
            }
        }

        // Subtle outer glow when ON
        Rectangle {
            id: fillGlow
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 0
            visible: control.checked
            layer.enabled: control.checked
            layer.effect: DropShadow {
                transparentBorder: true
                color: Qt.rgba(78/255, 91/255, 242/255, 0.42)
                samples: 22
                radius: 16
                horizontalOffset: 0
                verticalOffset: 0
            }
        }

        // Animations
        SequentialAnimation {
            id: shineAnim
            running: false
            PropertyAction { target: shine; property: "x"; value: -shine.width }
            PropertyAction { target: shine; property: "opacity"; value: 0.25 }
            NumberAnimation { target: shine; property: "x"; to: track.width; duration: 320; easing.type: Easing.OutCubic }
            NumberAnimation { target: shine; property: "opacity"; to: 0.0; duration: 140 }
        }
        SequentialAnimation {
            id: knobBounce
            running: false
            NumberAnimation { target: knob; property: "scale"; to: 1.08; duration: 90; easing.type: Easing.OutCubic }
            NumberAnimation { target: knob; property: "scale"; to: 1.0; duration: 130; easing.type: Easing.OutBack }
        }

        Connections {
            target: control
            function onToggled() {
                if (control.checked) shineAnim.restart()
                knobBounce.restart()
            }
        }
    }
}


