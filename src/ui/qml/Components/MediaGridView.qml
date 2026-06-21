import QtQuick
import ".."

GridView {
    id: grid

    property real imageAspectRatio: 319 / 225
    property real itemPerRow: Math.max(2, Math.floor(Globals.appWidth / 200))
    property real spacing: 12

    cellWidth: width / itemPerRow
    cellHeight: cellWidth * imageAspectRatio + Globals.sp(20) * 3

    reuseItems: true
    cacheBuffer: Math.round(cellHeight * 2)

    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    clip: true

    currentIndex: -1   // keyboard selection cursor; -1 hides the highlight
    highlightFollowsCurrentItem: true
    highlightMoveDuration: 130
    highlight: Rectangle {
        z: 2
        visible: grid.currentIndex >= 0
        color: "transparent"
        border.color: Theme.accent
        border.width: 2
        radius: 12
    }

    anchors.margins: spacing
    anchors.leftMargin: spacing
    anchors.rightMargin: spacing
    anchors.topMargin: spacing
}