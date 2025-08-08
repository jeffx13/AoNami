//a qmllint disable import unqualified
import QtQuick 2.15
import "../components"
import Kyokou.App.Main

MediaGridView {
    id: view
    onContentYChanged: watchListViewLastScrollY = view.contentY
    property real upperBoundary: 0.1 * view.height
    property real lowerBoundary: 0.9 * view.height

    signal dragFinished(int from, int to)
    signal contextMenuRequested(int index)

    // https://doc.qt.io/qt-6/qtquick-tutorials-dynamicview-dynamicview3-example.html
    // https://www.reddit.com/r/QtFramework/comments/1f1oikv/qml_drag_and_drop_with_gridview/

    model: DelegateModel {
        id: visualModel
        model: App.library.model
        delegate: DropArea {
            id: dropCell

            width: view.cellWidth
            height: view.cellHeight


            ShowItem {
                id: dragBox
                showTitle: model.title
                showCover: model.cover
                property int _index: model.index
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                onImageClicked: (mouse) => {
                                    if (mouse.button === Qt.LeftButton) {
                                        App.loadShow(index, true)
                                    } else {
                                        gridView.contextMenuRequested(index)
                                    }
                                }
                Drag.active: dragHandle.drag.active
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2
                width: dropCell.width; height: dropCell.height

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

                states: [
                    State {
                        when: dragBox.Drag.active

                        // disable anchors to allow dragBox to move
                        AnchorChanges {
                            target: dragBox
                            anchors.horizontalCenter: undefined
                            anchors.verticalCenter: undefined
                        }

                        // keep dragBox in front of other cells when dragging
                        ParentChange {
                            target: dragBox
                            parent: view
                        }
                    }
                ]

                MouseArea {
                    id: dragHandle
                    propagateComposedEvents: true
                    drag.target: dragBox
                    onReleased: dragBox.Drag.drop()

                    anchors.fill: parent
                    onMouseYChanged: (mouse) => {
                                         if (!drag.active) return
                                         let relativeY = gridView.mapFromItem(dragHandle, mouse.x, mouse.y).y
                                         if (!view.atYBeginning && relativeY < view.upperBoundary) {
                                             view.contentY -= 6
                                         } else if (!view.atYEnd && relativeY > view.lowerBoundary) {
                                             view.contentY += 6
                                         }
                                     }
                    cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.PointingHandCursor

                }
            }
            onDropped: function (drop) {
                let from = drop.source._index
                let to = model.index
                if (from === to) return
                view.dragFinished(from, to)
            }

            // onEntered: function (drag) {
            //     print("target:", drag.source._index, "into", model.index)
            //     visualModel.items.move(drag.source._index, model.index)
            // }

            states: [
                State {
                    when: dropCell.containsDrag && dropCell.drag.source != dragBox
                    PropertyChanges {
                        target: dropCell
                        opacity: 0.7
                    }
                }
            ]
        }
    }

}



