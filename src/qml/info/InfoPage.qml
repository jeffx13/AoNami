import QtQuick 2.15
import QtQuick.Controls 2.15
import "./../components"
Item{
    id:infoPage
    LoadingScreen{
        id:loadingScreen
        z:10
        anchors.fill: parent
        loading: app.playlistModel.loading
    }
    Connections{
        target: global.currentShowObject
        function onListTypeChanged(){
//            addToLibraryComboBox.model.setProperty(0,"text",)

        }
        function onShowChanged(){
            console.log("showchanged")
        }
    }
    focus: false
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
            source: global.currentShowObject.hasShow ? global.currentShowObject.coverUrl : "qrc:/resources/images/error_image.png"
            onStatusChanged: if (posterImage.status === Image.Null) source = "qrc:/resources/images/error_image.png"
            width: 200
            height: width * 432/305
            anchors{
                right: parent.right
                top: parent.top
            }
        }



        ComboBox{
            id:addToLibraryComboBox
            anchors{
                left:posterImage.left
                top: posterImage.bottom
            }
            enabled: global.currentShowObject.hasShow
            height: 50
            width: 200
            model: ListModel{
                ListElement { text: "" }
                ListElement { text: "Watching" }
                ListElement { text: "Planned" }
                ListElement { text: "On Hold" }
                ListElement { text: "Dropped" }
            }
            hoverEnabled: true
            currentIndex: global.currentShowObject.listType+1
            displayText: model.get(currentIndex).text.length !== 0 ? model.get(currentIndex).text : global.currentShowObject.isInWatchList ? "Remove from library" : "Add to library"

            delegate: ItemDelegate {
                text: model.text.length !== 0 ? model.text : global.currentShowObject.isInWatchList ? "Remove from library" : "Add to library"

                width: parent.width
                height: 30
                highlighted: hovered
                background: Rectangle {
                    color: index===addToLibraryComboBox.currentIndex ? "lightgreen" : hovered ? "lightblue" :"transparent"
                }
                enabled: !(addToLibraryComboBox.popup.opened && index === 0 && !global.currentShowObject.isInWatchList) || index !== 0

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if(index!==0){
                            app.watchList.addCurrentShow(index-1)
                        }else{
                            if(global.currentShowObject.isInWatchList){
                                app.watchList.removeCurrentShow()
                            }
                        }

                        addToLibraryComboBox.popup.close()
                    }
                }
            }
        }

        //        Button {
        //            id: addToListButton
        //            height: 50
        //            width: 200
        //            enabled: global.currentShowObject.hasShow
        //            text: global.currentShowObject.isInWatchList ? "added" : "add to list"
        //            onClicked: {
        //                if(!global.currentShowObject.isInWatchList){
        //                    app.watchList.addCurrentShow()
        //                    //                    console.log("added")
        //                    //                    console.log(global.currentShowObject.isInWatchList)
        //                }else{
        //                    app.watchList.removeCurrentShow()
        //                    //                    console.log("removed")
        //                    //                    console.log(global.currentShowObject.isInWatchList)
        //                }
        //            }

        //            anchors{
        //                left:posterImage.left
        //                top: posterImage.bottom
        //            }
        //        }
        Button{
            id:reverseButton
            anchors{
                left:addToLibraryComboBox.left
                top: addToLibraryComboBox.bottom
                right: addToLibraryComboBox.right
            }
            height: 50
            text: "reverse"
            onClicked: {
                app.episodeListModel.reversed = !app.episodeListModel.reversed
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
            text:"Continue from " + app.episodeListModel.continueEpisodeName
            height: 50
            onClicked: {
                app.loadSourceFromList(app.episodeListModel.continueIndex)
            }
        }
    }

    ListView{
        id:episodeListView
        clip: true
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
                if(app.episodeListModel.reversed){
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

