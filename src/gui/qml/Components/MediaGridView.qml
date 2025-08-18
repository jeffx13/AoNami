import QtQuick

GridView {
    id: gridView
    readonly property real imageAspectRatio: 319/225
    property real itemPerRow: Math.floor(root.width / 200)
    property real spacing: 16
    cellWidth: width / itemPerRow
    cellHeight: cellWidth * imageAspectRatio + 60 * root.fontSizeMultiplier

    boundsBehavior:Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    anchors.margins: spacing
    anchors.leftMargin: spacing
    anchors.rightMargin: spacing
    anchors.topMargin: spacing
    clip: true
}


