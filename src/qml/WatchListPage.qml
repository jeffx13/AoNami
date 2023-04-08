import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle{
    Component.onCompleted: {
        watchList.load()
    }
    Connections{
        target: watchList
        function onLoaded(showObject){
            var watchListJson = JSON.parse(showObject)
            for (let i = 0; i < watchListJson.length; i++) {
                const item = watchListJson[i];
                if(item.listType===0){
                    watchingListModel.append(item)
                }else{
                    plannedListModel.append(item)
                }
            }
        }
        function onPlannedAdded(showObject){
            plannedListModel.append(showObject)
        }

        function onWatchingAdded(showObject){

            watchingListModel.append(JSON.parse(showObject))
        }

        function onRemovedAtIndex(index){
            watchingList.model.remove(index)
        }
    }
    property real aspectRatio:319/225
    property real itemPerRow: 3
    GridView{
        id:watchingList
        clip: true
        anchors{
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        model:ListModel{
            id:watchingListModel
        }
        delegate:itemDelegate
        width: parent.width/2
        cellHeight: cellWidth * aspectRatio + 35
        cellWidth: width/itemPerRow
    }

    ListView{
        id:plannedList

        anchors{
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        //        cellHeight: cellWidth * aspectRatio + 35
        //        cellWidth: width/itemPerRow

        width: parent.width/2
        //        model:ListModel{
        //            id:plannedListModel
        //        }

        //        delegate: itemDelegate
        model: Qt.fontFamilies()

        delegate: Item {
            height: 40;
            width: ListView.view.width
            Text {
                anchors.centerIn: parent
                text: modelData;
                font.family: modelData
            }
        }
    }

    Component {
        id: itemDelegate
        Item {
            id: item
            Image {
                id:coverImage
                source: model.cover
                width: watchingList.cellWidth
                height: coverImage.width * aspectRatio

            }

            Text {
                text: model.title
                font.bold: ListView.isCurrentItem
                anchors.top: coverImage.bottom
                anchors.bottom: parent.bottom
                width: watchingList.cellWidth
                wrapMode: Text.Wrap
                font.pixelSize: 12
                height: contentHeight
                color: "white"

            }
            MouseArea{
                property var currentId;
                anchors.fill: coverImage
                onClicked: (mouse)=>{
                               watchList.loadDetails(index)
                           }
                //                onPressAndHold:{
                //                    console.log("held index: "+index)
                //                    currentId = index
                //                }
                //                onReleased: currentId = -1
                //                onPositionChanged: (mouse)=>{
                //                                       let newIndex = watchingList.indexAt(mouseX, mouseY)
                //                                       if (currentId !== -1 && index !== -1 && index !== newIndex)
                //                                       watchingList.model.move(newIndex, newIndex, 1)
                //                                       return
                //                                   }
            }

        }
    }


    color: "red"

}
