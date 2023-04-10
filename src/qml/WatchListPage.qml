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
        delegate:Rectangle{
            id:delegateRect
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
                //                anchors.bottom: parent.bottom
                width: watchingList.cellWidth
                wrapMode: Text.Wrap
                font.pixelSize: 12
                height: contentHeight
                color: "white"

            }

            MouseArea{
                id:mousearea
                property var currentId;
                anchors.fill: coverImage
                onClicked: (mouse)=>{
                               watchList.loadDetails(index)
                           }
                property bool followCursor: false

                drag {
                    id:dragging
                    property var originalPos
                    property var newPos
                    property var oldz
                    target: delegateRect
                    axis: Drag.XandYAxis
                    onActiveChanged: {
                        if(dragging.active){
                            dragging.originalPos = Qt.point(delegateRect.x, delegateRect.y)
                            oldz = delegateRect.z
                            delegateRect.z = 1000000000000
                        }else{
                            newPos = Qt.point(delegateRect.x, delegateRect.y)
                            let diff = Qt.point(newPos.x-dragging.originalPos.x, newPos.y-dragging.originalPos.y)
                            let dx = roundNearest(diff.x,watchingList.cellWidth)/watchingList.cellWidth
                            let dy = roundNearest(diff.y,watchingList.cellHeight)/watchingList.cellHeight
                            dx = dx<0 ? Math.ceil(dx) : Math.floor(dx)
                            dy = dy<0 ? Math.ceil(dy) : Math.floor(dy)
                            let newIndex = model.index+dx+dy*itemPerRow
                            delegateRect.z = oldz
                            if(newIndex===model.index){
                                watchingListModel.move(model.index, model.index+1,1)
                                watchingListModel.move(model.index, model.index-1,1)
                                return;
                            }
                            watchList.move(model.index,newIndex)
                            watchingListModel.move(model.index,newIndex,1)

                        }
                        function roundNearest(num,nearest){
                            return Math.round(num / nearest)*nearest;
                        }


                    }
                }


            }

        }

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



    color: "red"

}
