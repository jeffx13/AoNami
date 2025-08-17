pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import "../components"
import Kyokou.App.Main
Item {
    id: explorerPage

    SearchBar {
        id:searchBar
        anchors{
            left: parent.left
            right: parent.right
            top:parent.top
            topMargin: 10
        }
        focus: true
        height: parent.height * 0.08
    }

    MediaGridView {
        id:gridView
        model: App.searchResultModel
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
            width: 8
            contentItem: Rectangle {
                color: "#2F3B56"
                radius: width / 2
            }
            background: Rectangle {
                color: "#121826"
                radius: width / 2
            }
        }

        delegate: ShowItem {
            required property string title
            required property string link
            required property string cover
            required property int index
            showTitle: title
            showCover: cover
            width: gridView.cellWidth
            height: gridView.cellHeight
            onImageClicked: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    App.loadShow(index, false)
                } else if (mouse.button === Qt.RightButton){
                    contextMenu.index = index
                    contextMenu.popup()
                    contextMenu.libraryType = App.library.getLibraryType(link)
                    contextMenu.link = link
                } else if (mouse.button === Qt.MiddleButton) {
                    App.appendToPlaylists(index, false, false)
                }


            }
        }

        onContentYChanged: {
            if(atYEnd) {
                App.exploreMore(false)
            }
        }
        Component.onCompleted: {
            contentY = App.explorer.contentY
            forceActiveFocus()
        }
        Component.onDestruction: {
            App.explorer.contentY = contentY
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

    Menu {
        id: contextMenu
        modal: true
        Material.theme: Material.Dark
        Material.foreground: "#E5E7EB"
        Material.accent: "#4E5BF2"
        property int index
        property int libraryType
        property string link


        MenuItem {
            text: "Play"
            onTriggered:  {
                App.appendToPlaylists(contextMenu.index, false, true)
            }
        }

        MenuItem {
            text: "Append to Playlists"
            onTriggered:  {
                App.appendToPlaylists(contextMenu.index, false, false)
            }
        }

        MenuItem {
            id: removeMenuItem
            text: "Remove from Library"
            onTriggered:  {
                App.library.remove(contextMenu.link)
            }
            visible: contextMenu.libraryType !== -1
            height: visible ? implicitHeight : 0
        }

        Menu {
            id: addMenuItem
            title: contextMenu.libraryType === -1 ? "Add to Library" : "Change List Type"
            visible: contextMenu.libraryType === -1
            height: visible ? implicitHeight : 0
            MenuItem {
                text: "Watching"
                onTriggered: App.addToLibrary(contextMenu.index, 0)
                height: contextMenu.libraryType !== 0 ? implicitHeight : 0
            }
            MenuItem {
                text: "Planned"
                onTriggered: App.addToLibrary(contextMenu.index, 1)
                height: contextMenu.libraryType !== 1 ? implicitHeight : 0
            }
            MenuItem {
                text: "Paused"
                onTriggered: App.addToLibrary(contextMenu.index, 2)
                height: contextMenu.libraryType !== 2 ? implicitHeight : 0
            }
            MenuItem {
                text: "Dropped"
                onTriggered: App.addToLibrary(contextMenu.index, 3)
                height: contextMenu.libraryType !== 3 ? implicitHeight : 0
            }
            MenuItem {
                text: "Completed"
                onTriggered: App.addToLibrary(contextMenu.index, 4)
                height: contextMenu.libraryType !== 4 ? implicitHeight : 0
            }

        }
    }

    Keys.enabled: true
    Keys.onPressed: event => handleKeyPress(event)
    function handleKeyPress(event){
        if (event.modifiers & Qt.ControlModifier){
            if (event.key === Qt.Key_R){ App.exploreMore(true) }
        }else{
            switch (event.key){
            case Qt.Key_Escape:
            case Qt.Key_Alt:
                if (searchBar.textField.activeFocus)
                    explorerPage.forceActiveFocus()
                break;
            case Qt.Key_Tab:
                searchBar.providersBox.popup.close()
                App.providerManager.cycleProviders()
                event.accepted = true
                break;
            case Qt.Key_Enter:
                searchBar.search()
                break;
            case Qt.Key_Slash:
                searchBar.textField.forceActiveFocus()
                break;
            case Qt.Key_P:
                App.explore("", 1, false)
                break;
            case Qt.Key_L:
                App.explore("", 1, true)
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
