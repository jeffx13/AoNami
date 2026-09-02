import QtQuick
import ".."

TextEdit {
    readOnly: true
    selectByMouse: true
    textFormat: TextEdit.RichText
    wrapMode: TextEdit.Wrap
    color: Theme.textSecondary
    font.pixelSize: Globals.sp(20)

    onLinkActivated: (link) => Qt.openUrlExternally(link)
    HoverHandler {
        enabled: parent.hoveredLink.length > 0
        cursorShape: Qt.PointingHandCursor
    }
}
