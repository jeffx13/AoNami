// qmllint disable import unqualified
import QtQuick 2.15
import "../components"
import Kyokou.App.Main
MediaGridView {
    id: gridView
    onContentYChanged: watchListViewLastScrollY = gridView.contentY
    property real upperBoundary: 0.1 * gridView.height
    property real lowerBoundary: 0.9 * gridView.height
    property real lastY:0
    property int dragFromIndex: -1
    property int dragToIndex: -1
    signal moveRequested()
    signal contextMenuRequested(int index)
    property int movingIndex: -1
    // https://doc.qt.io/qt-6/qtquick-tutorials-dynamicview-dynamicview3-example.html

    model: DelegateModel {
        id: visualModel
        model: App.library.model
        property int heldZ: gridView.z + 10

        delegate: ShowItem {
            id: content
            width: gridView.cellWidth
            height: gridView.cellHeight
            Drag.active: dragArea.held
            Drag.source: dragArea
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2
            showTitle: model.title
            showCover: model.cover

            MouseArea {
                id: dragArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true
                drag.target: held ? content : undefined
                drag.axis: Drag.XAndYAxis
                cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.PointingHandCursor
                property bool held: false

                onClicked: (mouse) => {
                               if (mouse.button === Qt.LeftButton) {
                                   App.loadShow(index, true)
                               } else {
                                   gridView.contextMenuRequested(index)

                               }
                           }

                onPressed: (mouse) => {
                               if (mouse.button === Qt.LeftButton) {
                                   held=true
                               }
                           }

                onReleased: (mouse) => {
                                if (mouse.button === Qt.LeftButton) {
                                    held=false
                                }
                            }

                onMouseYChanged: (mouse) => {
                                     if (!drag.active) return
                                     let relativeY = gridView.mapFromItem(dragArea, mouse.x, mouse.y).y
                                     if (relativeY < gridView.upperBoundary && !gridView.atYBeginning) {
                                         gridView.contentY-=6;
                                     } else if (relativeY > gridView.lowerBoundary && !gridView.atYEnd) {
                                         gridView.contentY+=6;
                                     }
                                 }

                drag.onActiveChanged: {
                    if (drag.active) {
                        content.z = visualModel.heldZ
                        gridView.dragToIndex = model.index
                        gridView.dragFromIndex = model.index
                    } else {
                        content.z = gridView.z
                        visualModel.items.move(gridView.dragFromIndex, gridView.dragFromIndex)
                        gridView.lastY = gridView.contentY
                        gridView.moveRequested()
                    }
                }

                DropArea {
                    id:dropArea
                    anchors.fill:parent
                    anchors.margins: 10
                    onEntered: (drag) => {
                                   let oldIndex = drag.source.DelegateModel.itemsIndex
                                   let newIndex = dragArea.DelegateModel.itemsIndex
                                   gridView.dragToIndex = newIndex
                                   visualModel.items.move(oldIndex, newIndex)
                               }
                }

            }

        }

    }


}



// AnimatedImage {
//     anchors {
//         left:parent.left
//         right:parent.right
//         bottom:parent.bottom
//     }
//     source: "qrc:/resources/gifs/image-loading.gif"
//     width: parent.width
//     height: width * 0.84
//     visible: parent.status == Image.Loading
//     playing: parent.status == Image.Loading
// }

// Rectangle {
//     visible: unwatchedEpisodes !== 0
//     width: height * 1.5
//     height: parent.height / 10
//     color: "red"
//     radius: 1
//     anchors {
//         right: parent.right
//         top: parent.top
//     }

//     Text {
//         color: "white"
//         text: unwatchedEpisodes
//         font.pixelSize: 16
//     }
// }


