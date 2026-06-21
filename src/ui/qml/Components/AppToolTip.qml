import QtQuick
import QtQuick.Controls
import ".."

ToolTip {
    id: tip
    delay: 450
    padding: 7

    contentItem: Text {
        text: tip.text
        color: Theme.textPrimary
        font.family: Qt.application.font.family
        font.pixelSize: Globals.sp(16)
    }

    background: Rectangle {
        radius: 7
        color: "#F20A0F1E"
        border.color: Theme.border
        border.width: 1
    }
}
