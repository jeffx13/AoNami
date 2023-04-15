import QtQuick 2.15
import QtQuick.Controls 2.15
import "./components"
import "../components"

Item {
    id: searchPage
    property alias resultsList : list


    SearchBar{
        id:searchBar
        anchors{
            left: parent.left
            right: parent.right
            top:parent.top
        }
        height: 30
    }

    LoadingScreen{
        anchors.fill: parent
        loading: app.searchResultsModel.loading
    }

    GridView {
        id: list
        property real aspectRatio:319/225
        property real itemPerRow: 6
        boundsBehavior:Flickable.StopAtBounds
        anchors.top: searchBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        model: app.searchResultsModel
        cellHeight: cellWidth * aspectRatio + 35
        cellWidth: width/itemPerRow
        delegate:  itemDelegate
        highlight: highlight
        highlightFollowsCurrentItem: false
//        focus: true
        clip: true

//        footer: Rectangle{
//            color: "transparent"
//            width: parent.width
//            height: 100
//            z:list.z+1
//            BusyIndicator{
//                running: true
//                visible: true
//                width: width
//                height: parent.height
//                anchors.centerIn: parent

//            }
//        }

        interactive: true
        property int realContentHeight: Math.ceil(list.count/6)*cellHeight
        property int prevContentY

        onAtYEndChanged: {
            if(atYEnd && count > 0 && app.searchResultsModel.canLoadMore()){
                contentY = realContentHeight-height
                prevContentY=contentY
                app.searchResultsModel.loadMore();
            }
        }
        Connections{
            target: app.searchResultsModel
            function onPostItemsAppended(){
                if(list.prevContentY){
                    list.contentY = list.prevContentY+100

                }
            }
        }

        Component {
            id: itemDelegate
            Item {
                id: item
                Image {
                    id:coverImage
                    source:  model.cover// : "qrc:/kyokou/images/error_image.png"
                    onStatusChanged: if (coverImage.status === Image.Error) source = "qrc:/resources/images/error_image.png"
                    width: list.width/list.itemPerRow
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
                    MouseArea{
                        anchors.fill: parent
                        onClicked: (mouse)=>{
                                       app.searchResultsModel.loadDetails(index)
                                   }
                    }
                }
                Text {
                    text: model.title
                    font.bold: ListView.isCurrentItem
                    anchors.top: coverImage.bottom
                    anchors.bottom: parent.bottom
                    width: list.cellWidth
                    wrapMode: Text.Wrap
                    font.pixelSize: 12
                    height: contentHeight
                    color: "white"
                    MouseArea{
                        anchors.fill: parent
                        onClicked: (mouse)=>{
                                       if (mouse.button === Qt.RightButton){
                                           loadShowInfo()
                                       }
                                   }
                    }
                }
            }
        }



    }

}
