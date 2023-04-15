import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle{
    property var swipeView
    color: "black"



    Component {
        id: dragDelegate
        //https://doc.qt.io/qt-6/qtquick-tutorials-dynamicview-dynamicview3-example.html

        Rectangle {
            required property string title
            required property string cover
            required property int index

            id: content
            color: "red"
            Image {
                id:coverImage
                source: cover
                width: watchListView.cellWidth
                height: coverImage.width * aspectRatio
                anchors{
                    left: parent.left
                    top: parent.top
                }
            }

            Text {
                text: title
                font.bold: ListView.isCurrentItem
                anchors.top: coverImage.bottom
                //                anchors.bottom: parent.bottom
                width: watchListView.cellWidth
                wrapMode: Text.Wrap
                font.pixelSize: 12
                height: contentHeight
                color: "white"

            }

            width: watchListView.cellWidth
            height: watchListView.cellHeight

            Drag.active: dragArea.held
            Drag.source: dragArea
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2
            MouseArea {
                id: dragArea
                anchors {
                    fill: parent
                }
                onClicked: {
                    app.watchList.loadDetails(dragArea.DelegateModel.itemsIndex)
                }

                property int lastZ
                property bool held: true

                drag.target: held ? content : undefined
                drag.axis: Drag.XAndYAxis

                drag.onActiveChanged: {
                    if(drag.active){
                        lastZ = content.z
                        content.z = 10000000
                    }else{
                        content.z = lastZ
                        app.watchList.moveEnded()
                    }
                }

                DropArea {
                    anchors {
                        fill: parent
                        margins: 10
                    }

                    onEntered: (drag) => {
                                   let oldIndex = drag.source.DelegateModel.itemsIndex
                                   let newIndex = dragArea.DelegateModel.itemsIndex
                                   if(watchListView.lastLoadedIndex === oldIndex){
                                       watchListView.lastLoadedIndex = newIndex
                                   }
                                   let diff = Math.abs(newIndex-oldIndex)
                                   if(diff === 1 || diff === itemPerRow){
                                       //                                       console.log(oldIndex,newIndex)
                                       app.watchList.move(oldIndex,newIndex)
                                       visualModel.items.move(oldIndex,newIndex)
                                   }
                               }
                }
            }
        }
    }
    DelegateModel {
        id: visualModel

        model: app.watchList
        delegate: dragDelegate
    }
    property real aspectRatio:319/225
    property real itemPerRow: 6
    ComboBox{
        id:listTypeComboBox
        anchors{
            left: parent.left
            top: parent.top
        }
        width: 150
        height: 30
        model: ListModel{ListElement { text: "Watching" }ListElement { text: "Planned" }}
        hoverEnabled: true
        currentIndex: 0
        delegate: ItemDelegate {
            text: model.text
            width: parent.width
            height: 30
            highlighted: hovered
            background: Rectangle {
                color: highlighted ? "lightblue" : "transparent"
            }
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    listTypeComboBox.displayText = model.text
                    listTypeComboBox.popup.close()
                }
            }
        }
    }


    GridView{
        ScrollBar.vertical: ScrollBar {}
        id:watchListView
        clip: true
        anchors{
            left: parent.left
            top: listTypeComboBox.bottom
            bottom: parent.bottom
            right: parent.right
        }
        model: visualModel
        cellHeight: cellWidth * aspectRatio + 35
        cellWidth: width/itemPerRow
    }



}
