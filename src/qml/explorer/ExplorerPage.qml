import QtQuick
import QtQuick.Controls 2.15
import "../components"
Item {
    id: explorerPage

    SearchBar {
        id:searchBar
        anchors{
            left: parent.left
            right: parent.right
            top:parent.top
            topMargin: 5
        }
        focus: true
        height: parent.height * 0.06
    }

    MediaGridView {
        id:showGridView
        model: app.explorer
        focus: false
        anchors {
            top: searchBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        delegate: ShowItem {
            title: model.title
            cover: model.cover
            width: showGridView.cellWidth
            height: showGridView.cellHeight
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.PointingHandCursor
                onClicked: app.loadShow(index, false)
            }
        }

        onContentYChanged: {
            root.searchResultsViewlastScrollY = contentY
        }
        Component.onCompleted: {
            contentY = root.searchResultsViewlastScrollY
            forceActiveFocus()
        }

        add: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
            NumberAnimation { property: "scale"; from: 0; to: 1.0; duration: 400 }
        }

        displaced: Transition {
            NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.OutBounce }

            // ensure opacity and scale values return to 1.0
            NumberAnimation { property: "opacity"; to: 1.0 }
            NumberAnimation { property: "scale"; to: 1.0 }
        }
    }
    LoadingScreen {
        id:loadingScreen
        anchors.centerIn: parent
        loading: (app.explorer.isLoading || app.currentShow.isLoading) && explorerPage.visible
        onCancelled: {
            app.explorer.cancelLoading()
            app.currentShow.cancelLoading()
        }
        onTimedOut: {
            app.explorer.cancelLoading()
            app.currentShow.cancelLoading()
        }
    }



    Keys.enabled: true
    Keys.onPressed: event => handleKeyPress(event)
    function handleKeyPress(event){
        if (event.modifiers & Qt.ControlModifier){
            if (event.key === Qt.Key_R){app.explorer.reload()}
        }else{
            switch (event.key){
            case Qt.Key_Escape:
            case Qt.Key_Alt:
                if (searchBar.textField.activeFocus)
                    explorerPage.forceActiveFocus()
                break;
            case Qt.Key_Tab:
                searchBar.providersBox.popup.close()
                app.providerManager.cycleProviders()
                event.accepted = true
                break;
            case Qt.Key_Enter:
                searchBar.search()
                break;
            case Qt.Key_Slash:
                searchBar.textField.forceActiveFocus()
                break;
            case Qt.Key_P:
                app.popular(1)
                break;
            case Qt.Key_L:
                app.latest(1)
                break;
            case Qt.Key_Up:
                showGridView.flick(0,500)
                break;
            case Qt.Key_Down:
                showGridView.flick(0,-500)
                break;

            }
        }

    }
}
