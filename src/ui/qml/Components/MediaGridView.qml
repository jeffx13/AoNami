pragma ComponentBehavior: Bound
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

    function moveCursor(key) {
        if (grid.count <= 0) return false
        const step = key === Qt.Key_Up || key === Qt.Key_Down ? grid.itemPerRow : 1
        const back = key === Qt.Key_Left || key === Qt.Key_Up
        if (!back && key !== Qt.Key_Right && key !== Qt.Key_Down) return false
        grid.currentIndex = grid.currentIndex < 0 ? 0
                          : back                  ? Math.max(0, grid.currentIndex - step)
                                                  : Math.min(grid.count - 1, grid.currentIndex + step)
        return true
    }
}