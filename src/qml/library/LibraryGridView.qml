//a qmllint disable import unqualified
import QtQuick 2.15
import "../components"
import Kyokou.App.Main
MediaGridView {
    id: gridView
    onContentYChanged: watchListViewLastScrollY = gridView.contentY
    property real upperBoundary: 0.1 * gridView.height
    property real lowerBoundary: 0.9 * gridView.height
    property int initialDragIndex: -1
    property int currentDragIndex: -1
    interactive: currentDragIndex === -1

    signal dragFinished()
    signal contextMenuRequested(int index)
    property int movingIndex: -1
    // https://doc.qt.io/qt-6/qtquick-tutorials-dynamicview-dynamicview3-example.html

    model: DelegateModel {
        id: visualModel
        model: App.library.model

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
            Rectangle {
                anchors {
                    top: parent.top
                    right:parent.right
                }
                width: 40
                height: width
                radius: width / 2
                color: "white"
                border.color: "red"
                border.width: 5
                Text {
                    text: model.unwatchedEpisodes
                    color: "black"
                    anchors.centerIn: parent
                }
                visible: model.unwatchedEpisodes > 0
            }

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
                                     // console.log(gridView.contentY, (gridView.contentHeight- gridView.height), gridView.contentY/(gridView.contentHeight- gridView.height))

                                     let relativeY = gridView.mapFromItem(dragArea, mouse.x, mouse.y).y
                                     if (!gridView.atYBeginning && relativeY < gridView.upperBoundary) {
                                         gridView.contentY -= 6
                                     } else if (!gridView.atYEnd && relativeY > gridView.lowerBoundary) {
                                         gridView.contentY += 6
                                     }
                                 }

                drag.onActiveChanged: {
                    if (drag.active) {
                        content.z = 666 // bring to the top
                        gridView.initialDragIndex = model.index
                        gridView.currentDragIndex = model.index
                    } else {
                        content.z = gridView.z // bring back
                        gridView.dragFinished()
                    }
                }

                DropArea {
                    id:dropArea
                    anchors.fill:parent
                    anchors.margins: 10
                    onEntered: (drag) => {
                                   let newIndex = dragArea.DelegateModel.itemsIndex
                                   if (gridView.currentDragIndex === newIndex) return;
                                   gridView.currentDragIndex = newIndex
                                   let oldIndex = drag.source.DelegateModel.itemsIndex

                               }
                }

            }

        }

    }


}



