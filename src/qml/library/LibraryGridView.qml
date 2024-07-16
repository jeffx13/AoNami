import QtQuick 2.15
import QtQuick.Controls
import "../components"
MediaGridView {
    id: gridView
    onContentYChanged: watchListViewLastScrollY = contentY
    property real upperBoundary: 0.1 * gridView.height
    property real lowerBoundary: 0.9 * gridView.height
    property real lastY:0
    signal moveRequested()
    property int dragFromIndex: -1
    property int dragToIndex: -1
    // https://doc.qt.io/qt-6/qtquick-tutorials-dynamicview-dynamicview3-example.html
    model: DelegateModel {
        id: visualModel
        model: app.library.model
        property int heldZ: gridView.z + 10

        delegate: ShowItem {
            id: content
            width: gridView.cellWidth
            height: gridView.cellHeight
            Drag.active: dragArea.held
            Drag.source: dragArea
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2
            title: model.title
            cover: model.cover
            // required property int index
            // required property int unwatchedEpisodes


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
                                   app.loadShow(model.index, true)
                               } else {
                                   contextMenu.index = index
                                   contextMenu.popup()
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
                        moveRequested()
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

    Menu {
        id: contextMenu
        modal: true
        property int index
        MenuItem {
            text: "Remove from library"
            onTriggered:  {
                app.library.removeAt(contextMenu.index)
            }
        }

        Menu {
            title: "Change list type"
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 0
                text: "Watching"
                onTriggered: app.library.changeListTypeAt(contextMenu.index, 0, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 1
                text: "Planned"
                onTriggered: app.library.changeListTypeAt(contextMenu.index, 1, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 2
                text: "On Hold"
                onTriggered: app.library.changeListTypeAt(contextMenu.index, 2, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 3
                text: "Dropped"
                onTriggered: app.library.changeListTypeAt(contextMenu.index, 3, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 4
                text: "Completed"
                onTriggered: app.library.changeListTypeAt(contextMenu.index, 4, -1)
                height: visible ? implicitHeight : 0
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


