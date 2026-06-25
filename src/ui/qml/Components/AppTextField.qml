import QtQuick.Controls
import QtQuick
import ".."

TextField {
    id: field

    property color checkedColor: Theme.border
    property color accentColor: Theme.accent
    property color surfaceColor: Theme.surface
    property int fontSize: 20
    property bool showClearButton: false

    font.family: "QTxiaotu"
    font.pixelSize: Globals.sp(fontSize)
    color: Theme.textPrimary
    placeholderTextColor: Theme.textMuted
    selectionColor: accentColor
    selectedTextColor: "#ffffff"
    hoverEnabled: true
    leftPadding: 14
    rightPadding: (showClearButton && text.length > 0) ? clearBtn.width + 8 : 14

    // Esc unfocuses and is consumed here (won't reach the page, e.g. exit fullscreen).
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape) {
            event.accepted = true
            field.focus = false
        }
    }
    onActiveFocusChanged: if (!activeFocus) field.unfocused()
    signal unfocused()

    background: Rectangle {
        radius: 10
        color: field.activeFocus ? Theme.surfaceAlt : field.surfaceColor
        border.color: field.activeFocus ? field.accentColor
                    : field.hovered     ? Qt.lighter(field.accentColor, 1.3)
                    : field.checkedColor
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: 150 } }
        Behavior on color { ColorAnimation { duration: 150 } }

        Rectangle {
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
                bottomMargin: -1
            }
            height: 2
            radius: 1
            width: field.activeFocus ? parent.width - 20 : 0
            color: field.accentColor
            opacity: field.activeFocus ? 1.0 : 0.0
            Behavior on width {
                NumberAnimation {
                    duration: 250
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }

        Rectangle {
            visible: field.activeFocus
            anchors {
                fill: parent
                margins: -3
            }
            radius: parent.radius + 3
            color: "transparent"
            border.color: Qt.alpha(Theme.accent, 0.13)
            border.width: 2
            opacity: field.activeFocus ? 1.0 : 0.0
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
    }

    AbstractButton {
        id: clearBtn
        visible: field.showClearButton && field.text.length > 0
        width: 28
        height: parent.height
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        focusPolicy: Qt.NoFocus

        contentItem: Item {
            AppIcon {
                anchors.centerIn: parent
                name: "x"
                size: 14
                color: clearBtn.hovered ? Theme.textPrimary : Theme.textMuted
            }
        }

        background: Item {}

        onClicked: field.clear()
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        acceptedButtons: Qt.NoButton
    }
}
