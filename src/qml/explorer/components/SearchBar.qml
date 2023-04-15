import QtQuick 2.15
import QtQuick.Controls 2.15
Item {
    id:searchBar

    TextField{
        id:searchTextField
        placeholderText: qsTr("Enter anime!")
        anchors.top:searchBar.top
        anchors.left: searchBar.left
        background: Rectangle{
            color: "white"
        }
        visible: true
        height: searchBar.height
        width: searchBar.width/2
        onAccepted: {
            app.searchResultsModel.search(searchTextField.text,1,1)
            resultsList.prevContentY=0
        }
    }

    Button{
        id: searchButton
        anchors{
            top: searchBar.top
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
        focusPolicy: Qt.NoFocus
        onClicked: {
            app.searchResultsModel.search(searchTextField.text,1,1)
            resultsList.prevContentY=0
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
        focusPolicy: Qt.NoFocus
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
            resultsList.prevContentY=0
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
        focusPolicy: Qt.NoFocus
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
            resultsList.prevContentY=0
        }
    }

    ComboBox{
        id:providersComboBox
        anchors{
            top: searchBar.top
            left: popularButton.right
            bottom: popularButton.bottom
            right: searchBar.right
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


}
