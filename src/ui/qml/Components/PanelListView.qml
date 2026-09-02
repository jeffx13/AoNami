import QtQuick
import QtQuick.Controls
import ".."

ListView {
    id: view

    property string emptyText: ""

    clip: true
    boundsBehavior: Flickable.StopAtBounds
    spacing: 2

    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
        width: 4
        contentItem: Rectangle { color: Theme.accent; radius: 2; opacity: 0.4 }
    }

    Text {
        anchors.centerIn: parent
        visible: view.count === 0
        text: view.emptyText
        color: Theme.onOverlayDim
        font.pixelSize: Globals.sp(20)
    }
}
