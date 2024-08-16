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
        id:gridView
        model: app.explorer
        focus: false
        anchors {
            top: searchBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            rightMargin: 20
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            parent: gridView.parent
            anchors.top: gridView.top
            anchors.left: gridView.right
            anchors.bottom: gridView.bottom
            width: 20
            contentItem: Rectangle {

                radius: width / 2
            }
            background: Rectangle {

                radius: width / 2
                color: 'transparent'
            }
        }

        delegate: ShowItem {
            title: model.title
            cover: model.cover
            width: gridView.cellWidth
            height: gridView.cellHeight
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.PointingHandCursor
                onClicked: app.loadShow(model.index, false)
            }
        }

        onContentYChanged: {
            if(atYEnd) {
                app.exploreMore(false)
            }
        }
        Component.onCompleted: {
            contentY = app.explorer.contentY
            forceActiveFocus()
        }
        Component.onDestruction: {
            app.explorer.contentY = contentY
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


    Keys.enabled: true
    Keys.onPressed: event => handleKeyPress(event)
    function handleKeyPress(event){
        if (event.modifiers & Qt.ControlModifier){
            if (event.key === Qt.Key_R){ app.exploreMore(true) }
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
                app.explore("", 1, false)
                break;
            case Qt.Key_L:
                app.explore("", 1, true)
                break;
            case Qt.Key_Up:
                gridView.flick(0,500)
                break;
            case Qt.Key_Down:
                gridView.flick(0,-500)
                break;

            }
        }

    }
}
