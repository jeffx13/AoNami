import QtQuick 2.15
import QtQuick.Controls 2.15
GridView {
    id: list
    property real aspectRatio:319/225
    property real itemPerRow: 6
    property int lastIndex


    populate: Transition {
        OpacityAnimator {from: 0; to: 1; duration: 500}
    }
    add: Transition {
        OpacityAnimator {from: 0; to: 1; duration: 500}
    }
    remove: Transition {
        OpacityAnimator {from: 1; to: 0; duration: 250}
    }

    ScrollBar.vertical: ScrollBar {
        //            visible: explorer.advancedSearch.width == 300 || explorer.advancedSearch.width == 0
        active: hovered || pressed
        anchors.right: parent.right
        width: 20
        hoverEnabled: true
    }

        footer: busyFooter
        Component{
            id:busyFooter
            Rectangle{
                BusyIndicator {
                    width: footerHeight
                    visible: list.count > 0
                    running: true//explorer.grid.count > 0 && !explorer.endOfResults
                    height: footerHeight
                    anchors.centerIn: parent
                }
                color: "transparent"
                width: parent.width
                height: footerHeight
                visible: list.atYEnd
            }
        }
        property real footerHeight:100

    //    interactive: true

    boundsBehavior: Flickable.DragOverBounds


    model: app.searchResultsModel
    cellHeight: cellWidth * aspectRatio + 35
    cellWidth: width/itemPerRow
    delegate:Rectangle {
        id: item
        //            width: list.cellWidth - 20
        //            height: list.cellHeight - 10
        color: "transparent"
        Image {
            id:coverImage
            source: model.cover
            onStatusChanged: if (coverImage.status === Image.Error){
                                 source = "qrc:/kyokou/images/error_image.png"
                             }

            width: list.cellWidth
            height: coverImage.width * list.aspectRatio
            BusyIndicator {
                id: busyIndicator
                anchors.centerIn: parent
                running: coverImage.status == Image.Loading
                visible: coverImage.status == Image.Loading
                z:-1 //100
                height: 70
                width: 70
            }

        }

        Text {
            text: model.title
            font.bold: ListView.isCurrentItem
            anchors{
                top: coverImage.bottom
                bottom: parent.bottom
                left: coverImage.left
                right: coverImage.right
            }

            //            width: list.cellWidth
            wrapMode: Text.Wrap
            font.pixelSize: 12
            //            height: contentHeight
            color: "white"
            MouseArea{
                anchors.fill: parent
                onClicked: (mouse)=>{
                               if (mouse.button === Qt.RightButton){
                                   //                                       loadShowInfo()
                               }
                           }

            }
        }

        MouseArea{
            anchors.fill: coverImage
            z:parent.z+1
            onClicked: (mouse)=>{

                           //                               app.searchResultsModel.loadDetails(index)
                           if(list.contentY-list.originY>0)list.lastY = list.contentY-list.originY
                           console.log(list.lastY)
                           app.searchResultsModel.loadMore()
                       }
            cursorShape: Qt.PointingHandCursor
        }
    }

    highlight: highlight
    highlightFollowsCurrentItem: false
    focus: true
    clip: true

    property real lastY: 0
    //    onAtYEndChanged: {
    //        if (atYEnd && count>0 && model.canFetchMore()) {
    //            //            list.lastIndex = list.count-1
    //            currentIndex = list.count - 1

    //            list.lastY = contentY-655
    //            console.log(lastY)

    //            //            console.log("set lastY",list.lastY)
    //            app.searchResultsModel.fetchMore()
    //        }
    //    }

    //    interactive: false
    property int realContentHeight: Math.ceil(list.count/list.itemPerRow)*list.cellHeight
    property int prevContentY
    property bool canFetch: true
    //    interactive: false
    boundsMovement: GridView.StopAtBounds
    onAtYEndChanged: {
        //        console.log(atYEnd,canFetch)
        if (atYEnd && canFetch){
            canFetch = false
            lastIndex = list.count-1
            //            if(contentY-originY>0)lastY = contentY-originY
            lastY = contentY - originY
//            console.log("set lastY",lastY,"contentheight",contentHeight-height)
            //            //            console.log(contentY-originY,contentHeight-height)
            app.searchResultsModel.loadMore()

                        console.log("called")
        }
    }
    //    WheelArea {
    //        width: parent.width - parent.x
    //    }




    onContentYChanged: {
        if(!moving && (lastIndex !== (list.count-1))){
//            console.log("why!!!")

//            console.log("back to lastY",(lastY+originY),contentHeight)
//            contentY = lastY + originY
            console.log("back to index",lastIndex)
            list.positionViewAtIndex(lastIndex,GridView.End)
            //            canFetch = true
        }else if(moving){
//            console.log("moving")
            canFetch = true
        }

        //        lastY = contentY-originY
    }




    property bool layoutJustChanged: false
    Connections{
        target: app.searchResultsModel

        function onPostItemsAppended(){
            //            contentY = lastY+originY
            //            list.contentY = list.prevContentY

        }
        function onLayoutChanged(){
            //            console.log("layout changed")
            //            console.log(count)
            //            layoutJustChanged = true
            //            canFetch = true
            //            list.positionViewAtIndex(list.count-1,GridView.Beginning)

        }
    }






}
