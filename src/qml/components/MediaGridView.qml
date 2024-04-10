import QtQuick

GridView {
    id: gridView
    readonly property real imageAspectRatio: 319/225
    property real itemPerRow: Math.floor(root.width/(root.maximised ? 300 : 200))
    property real spacing: 10
    cellWidth: width / itemPerRow
    cellHeight: cellWidth * imageAspectRatio + 60 * root.fontSizeMultiplier

    boundsBehavior:Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    anchors.topMargin: spacing
    clip: true





    // onAtYEndChanged: {
    //     if (atYEnd && count > 0 && app.explorer.canLoadMore()) {
    //         app.explorer.loadMore();
    //     }
    // }
}


