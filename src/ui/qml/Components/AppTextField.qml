import QtQuick.Controls
import QtQuick
import ".."

TextField {
    id: field

    property color checkedColor: Theme.border
    property int fontSize: 20
    property bool showClearButton: false

    font.family: Globals.fontFamily
    font.pixelSize: Globals.sp(fontSize)
    color: Theme.textPrimary
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.accent
    selectedTextColor: Theme.onColor(Theme.accent)
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

    background: FieldBackground {
        focused: field.activeFocus
        hovered: field.hovered
        restingBorder: field.checkedColor

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
