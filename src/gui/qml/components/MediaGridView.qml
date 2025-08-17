import QtQuick

GridView {
    id: gridView
    readonly property real imageAspectRatio: 319/225
    property real itemPerRow: Math.floor(root.width/(root.maximised ? 300 : 220))
    property real spacing: 16
    cellWidth: width / itemPerRow
    cellHeight: cellWidth * imageAspectRatio + 68 * root.fontSizeMultiplier

    boundsBehavior:Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    anchors.margins: spacing
    anchors.leftMargin: spacing
    anchors.rightMargin: spacing
    anchors.topMargin: spacing
    clip: true
}


