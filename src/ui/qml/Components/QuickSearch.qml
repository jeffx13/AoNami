pragma ComponentBehavior: Bound
import QtQuick
import ".."
import AoNami

Rectangle {
    id: quickSearch

    property bool open: false
    signal searched(string query)

    color: Theme.scrim
    visible: opacity > 0.01
    opacity: open ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 120 } }
    onOpenChanged: if (open) { qsField.text = ""; qsField.forceActiveFocus() }

    MouseArea { anchors.fill: parent; onClicked: quickSearch.open = false }

    Rectangle {
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: parent.height * 0.18 }
        width: Math.min(560, parent.width - 80)
        height: 58
        radius: 14
        color: Theme.surface
        border.color: Theme.accent
        border.width: 1

        MouseArea { anchors.fill: parent }   // swallow clicks so the box stays open

        AppIcon {
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 16 }
            name: "search"
            color: Theme.textMuted
            size: 20
        }
        AppTextField {
            id: qsField
            anchors { fill: parent; leftMargin: 50; rightMargin: 16 }
            leftPadding: 0; rightPadding: 0
            placeholderText: qsTr("Search anime, shows, movies...")
            background: null
            onAccepted: {
                if (text.trim().length === 0) return
                quickSearch.searched(text)
                quickSearch.open = false
            }
            // AppTextField consumes Escape to drop focus; closing on that is what Escape means here.
            onUnfocused: quickSearch.open = false
        }
    }
}
