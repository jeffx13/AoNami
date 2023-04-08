import QtQuick 2.15
import QtQuick.Controls 2.15
Item {
    id: searchPage
    function search(){
        app.searchResultsModel.search(searchTextField.text,1,1)
        list.prevContentY=0
    }

    TextField{
        id:searchTextField
        placeholderText: qsTr("Enter anime!")
        anchors.top:parent.top
        anchors.left: parent.left
        background: Rectangle{
            color: "white"
        }
        height: 30
        width: parent.width/2
        onAccepted: searchPage.search()
    }
    Button{
        id: searchButton
        anchors{
            top: parent.top
            left: searchTextField.right
            bottom: searchTextField.bottom
        }
        background: Rectangle {
            color: searchButton.enabled ? "black" : "grey"
            radius: 5
        }
        width: textItem.implicitWidth + 20
        text: "search"
        contentItem: Text {
            id: textItem
            text: searchButton.text
            font: searchButton.font
            color: searchButton.enabled ? "white" : "grey"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        onClicked: {
            searchPage.search()
        }
    }
    Button{
        id: latestButton
        anchors{
            top: parent.top
            left: searchButton.right
            bottom: searchButton.bottom
        }
        background: Rectangle {
            color: latestButton.enabled ? "black" : "grey"
            radius: 5
        }
        width: textItem1.implicitWidth + 20
        text: "latest"
        contentItem: Text {
            id: textItem1
            text: latestButton.text
            font: latestButton.font
            color: latestButton.enabled ? "white" : "grey"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        onClicked: {
            app.searchResultsModel.latest(1,3)
            list.prevContentY=0
        }
    }
    Button{
        id: popularButton
        anchors{
            top: parent.top
            left: latestButton.right
            bottom: latestButton.bottom
        }
        background: Rectangle {
            color: searchButton.enabled ? "black" : "grey"
            radius: 5
        }
        width: textItem2.implicitWidth + 20
        text: "popular"
        contentItem: Text {
            id: textItem2
            text: popularButton.text
            font: popularButton.font
            color: popularButton.enabled ? "white" : "grey"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        onClicked: {
            app.searchResultsModel.popular(1,3)
            list.prevContentY=0
        }
    }

    ComboBox{
        id:providersComboBox
        anchors{
            top: parent.top
            left: popularButton.right
            bottom: popularButton.bottom
            right: parent.right
        }
        model: ListModel{}
        hoverEnabled: true
        currentIndex: 0
        delegate: ItemDelegate {
            text: model.name
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
                    global.changeSearchProvider(model.providerEnum)
                    providersComboBox.displayText = model.name
                    providersComboBox.popup.close()
                }
            }
        }
        Component.onCompleted: {
            global.providers.forEach(function(provider) {
                providersComboBox.model.append({name:provider.name,providerEnum:provider.providerEnum})
            })
            providersComboBox.displayText = global.currentSearchProvider.name


        }
    }

    GridView {
        id: list
        property real aspectRatio:319/225
        property real itemPerRow: 6

        anchors.top: searchTextField.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        model: app.searchResultsModel
        cellHeight: list.cellWidth * aspectRatio + 35
        cellWidth: list.width/itemPerRow
        delegate:  itemDelegate
        highlight: highlight
        highlightFollowsCurrentItem: false
        focus: true
        clip: true
        interactive: true
        property int realContentHeight: Math.ceil(list.count/6)*cellHeight
        property int prevContentY
        onContentYChanged: {
            if (parseInt(contentY+list.height) >= realContentHeight+150) {
                if (model.canFetchMore()) {
                    contentY =realContentHeight-list.height
                    prevContentY=contentY
                    model.fetchMore();
                }
            }
        }

    }
    Connections{
        target: app.searchResultsModel
        function onPostItemsAppended(){
            list.contentY = list.prevContentY
        }
    }

    Component {
        id: itemDelegate
        Item {
            id: item


            Image {
                id:coverImage
                source: model.cover
                onStatusChanged: if (coverImage.status === Image.Error) source = "qrc:/Bingime/images/error_image.png"
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

    Component.onCompleted: app.searchResultsModel.latest(1,3)
}
