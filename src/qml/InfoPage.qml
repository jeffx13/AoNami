import QtQuick 2.15
import QtQuick.Controls 2.15
Item{
    id:infoPage

    Rectangle{
        id:infoBackgroundRect
        anchors{
            top: parent.top
            bottom: parent.bottom
            left:parent.left
        }
        width: parent.width * 0.8
        color: "white"
        Column{
            spacing: 10
            anchors{
                right: posterImage.left
                top: parent.top
                left: parent.left
            }

            Text {
                id:titleText
                text: global.currentShowObject.title
                font.pixelSize: 24

            }

            Text {
                id:descText
                text: global.currentShowObject.desc

                anchors{
                    left: parent.left
                    right: parent.right
                }
                wrapMode: Text.Wrap
                font.pixelSize: 16
            }
            Row {
                id:ratingViewsText
                spacing: 10

                Text {
                    id:ratingText
                    text: "Rating: " + global.currentShowObject.rating + "/10"
                    font.pixelSize: 16

                }

                Text {
                    text: "Views: " + global.currentShowObject.views.toLocaleString()
                    font.pixelSize: 16
                }
            }
            Text {
                id:yearText

                text: "Year: " + global.currentShowObject.year
                font.pixelSize: 16
            }

            Text {
                id:statusText

                text: "Status: " + global.currentShowObject.status
                font.pixelSize: 16
            }

            Text {
                id:genresText


                text: "Genres: " + global.currentShowObject.genresString
                font.pixelSize: 16
            }

            Text {

                text: "Update Time: " + global.currentShowObject.updateTime
                font.pixelSize: 16
            }
        }
        Image {
            id: posterImage
            source: global.currentShowObject.coverUrl
            //            onStatusChanged: if (posterImage.status === Image.Error) source = "qrc:/Bingime/images/error_image.png"
            width: 200
            height: width * 432/305
            anchors{
                right: parent.right
                top: parent.top
            }

        }
        Button {
            id: addToListButton
            height: 50
            width: 200

            text: global.currentShowObject.isInWatchList ? "added" : "add to list"

            onClicked: {

                if(!global.currentShowObject.isInWatchList){
                    watchList.addCurrentShow()
//                    console.log("added")
//                    console.log(global.currentShowObject.isInWatchList)
                }else{
                    watchList.removeCurrentShow()
//                    console.log("removed")
//                    console.log(global.currentShowObject.isInWatchList)
                }

            }

            anchors{
                left:posterImage.left
                top: posterImage.bottom
            }
        }
        Button{
            id:reverseButton
            anchors{
                left:addToListButton.left
                top: addToListButton.bottom
                right: addToListButton.right
            }
            height: 50
            text: "reverse"
            onClicked: {
                app.episodeListModel.reversed = !app.episodeListModel.reversed
                episodeListView.reversed = app.episodeListModel.reversed
            }
        }
        Button{
            id:continueWatchingButton
            visible: global.currentShowObject.lastWatchedIndex !== -1
            anchors{
                left:reverseButton.left
                top: reverseButton.bottom
                right: reverseButton.right
            }
            text:"Continue from " + app.episodeListModel.lastWatchedEpisodeName
            height: 50
            onClicked: {
                app.loadSourceFromList(global.currentShowObject.lastWatchedIndex)
            }
        }
    }

    ListView{
        id:episodeListView
        clip: true
        property bool reversed;
        anchors{
            left:infoBackgroundRect.right
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        model:app.episodeListModel

        delegate: Rectangle {
            id: delegateRect
            width: episodeListView.width
            height: 50 < (episodeStr.height + 10) ? (episodeStr.height + 10) : 50
            color: {
                let lastWatchedIndex = global.currentShowObject.lastWatchedIndex;
                if(lastWatchedIndex === -1)return "#f2f2f2"
                if(episodeListView.reversed){
                    lastWatchedIndex = episodeListView.count - lastWatchedIndex - 1
                }
                if(lastWatchedIndex === index){
//                    continueWatchingButton.text ="Continue from " + model.number.toString() + (model.title === undefined || parseInt(model.title) === model.number ? "" : "\n" + model.title)
                    return "red"
                }
                return "#f2f2f2"
            }
            border.color: "#ccc"
            border.width: 1
            radius: 5
            Text {
                id:episodeStr
                text:  model.number.toString() + (model.title === undefined || parseInt(model.title) === model.number ? "" : "\n" + model.title)
                font.pixelSize: 16
                anchors{
                    left:parent.left
                    right:parent.right
                    top:parent.top
                }
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: "#444"
                font.family: "Arial"
            }



            MouseArea {
                anchors.fill: delegateRect
                //                onEntered: delegateRect.color = "#ccc"
                //                onExited: delegateRect.color = "#f2f2f2"
                onClicked: (mouse)=>{
                               app.loadSourceFromList(index)
                           }
            }

        }
    }

}

