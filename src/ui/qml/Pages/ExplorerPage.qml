pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import AoNami
import ".."


Item {
    id: explorerPage
    function search() { App.explore(searchTextField.text, 1, false) }
    focus: true
    Component.onDestruction: {
        Globals.lastSearch = searchTextField.text
        Globals.explorerLastContentY = gridView.contentY
    }

    RowLayout {
        id: searchBar
        focus: false
        height: parent.height * 0.08
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: 10
        }

        AppTextField {
            id: searchTextField
            color: "white"
            checkedColor: "#727CF5"
            placeholderText: qsTr("Enter query!")
            placeholderTextColor: "gray"
            text: Globals.lastSearch
            fontSize: 20
            focusPolicy: Qt.NoFocus
            focus: false
            activeFocusOnTab: false

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 5

            onAccepted: search()
        }

        AppButton {
            id: searchButton
            text: "Search"
            fontSize: 20
            radius: 20
            focusPolicy: Qt.NoFocus
            focus: false
            activeFocusOnTab: false

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 1

            onClicked: search()
        }

        AppButton {
            id: latestButton
            text: "Latest"
            radius: 20
            focusPolicy: Qt.NoFocus
            focus: false
            activeFocusOnTab: false

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 1

            onClicked: App.explore("", 1, true)
        }

        AppButton {
            id: popularButton
            text: "Popular"
            radius: 20
            focusPolicy: Qt.NoFocus
            focus: false
            activeFocusOnTab: false

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 1

            onClicked: App.explore("", 1, false)
        }

        AppComboBox {
            id: providerComboBox
            text: "text"
            fontSize: 20
            model: App.providerManager
            focus: false
            currentIndex: App.providerManager.currentProviderIndex
            activeFocusOnTab: false

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 2

            onActivated: (index) => {
                App.providerManager.currentProviderIndex = index
            }
        }

        AppComboBox {
            id: typeComboBox
            text: ""
            // Removed unused contentRadius
            fontSize: 20
            focus: false
            model: App.providerManager.availableShowTypes
            currentIndex: App.providerManager.currentSearchTypeIndex
            currentIndexColor: "red"
            activeFocusOnTab: false

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 2

            onActivated: (index) => {
                App.providerManager.currentSearchTypeIndex = index
            }
        }
    }

    MediaGridView {
        id: gridView
        model: App.searchResultModel
        focus: false
        anchors {
            top: searchBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            rightMargin: 20
        }
        imageAspectRatio: Globals.imageAspectRatio
        Component.onCompleted: contentY = Globals.explorerLastContentY
        onContentYChanged: {
            if (contentY + gridView.height >= gridView.contentHeight * 0.8) {
                App.explorer.fetchMore()
            }
        }
        onContentHeightChanged: if (contentHeight < height) App.explorer.fetchMore()

        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            policy: ScrollBar.AsNeeded
            parent: gridView.parent
            width: 8

            anchors {
                top: gridView.top
                left: gridView.right
                bottom: gridView.bottom
            }

            contentItem: Rectangle {
                color: "#2F3B56"
                radius: width / 2
            }

            background: Rectangle {
                color: "#121826"
                radius: width / 2
            }
        }

        onImageAspectRatioChanged: {
            let lastContentY = contentY
            App.searchResultModel.reset()
            contentY = lastContentY
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
            aspectRatio: Globals.imageAspectRatio
            onImageLoaded: (sourceAspectRatio) => {
                if (index !== 0) return
                if (Math.abs(Globals.imageAspectRatio - sourceAspectRatio) < 0.01) {
                    return
                }
                Globals.imageAspectRatio = sourceAspectRatio
            }

            onImageClicked: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    App.loadShow(index, false)
                } else if (mouse.button === Qt.RightButton) {
                    contextMenu.index = index
                    contextMenu.libraryType = App.library.getLibraryType(link)
                    contextMenu.link = link
                    contextMenu.popup()
                } else if (mouse.button === Qt.MiddleButton) {
                    App.appendToPlaylists(index, false, false)
                }
            }
        }




        add: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1.0
                duration: 400
            }
            NumberAnimation {
                property: "scale"
                from: 0
                to: 1.0
                duration: 400
            }
        }



        displaced: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 400
                easing.type: Easing.OutBounce
            }

            NumberAnimation {
                property: "opacity"
                to: 1.0
            }
            NumberAnimation {
                property: "scale"
                to: 1.0
            }
        }

    }

    AppMenu {
        id: contextMenu
        modal: true
        property int index
        property int libraryType
        property string link

        Action {
            text: "Play"
            onTriggered: App.appendToPlaylists(contextMenu.index, false, true)
        }

        Action {
            text: "Append To Playlists"
            onTriggered: App.appendToPlaylists(contextMenu.index, false, false)
        }

        AppMenu {
            title: contextMenu.libraryType === -1 ? "Add To Library" : "Change Library Type"
            Action {
                text: "Watching"
                enabled: contextMenu.libraryType !== 0
                onTriggered: App.addToLibrary(contextMenu.index, 0)
            }

            Action {
                text: "Planned"
                enabled: contextMenu.libraryType !== 1
                onTriggered: App.addToLibrary(contextMenu.index, 1)
            }

            Action {
                text: "Paused"
                enabled: contextMenu.libraryType !== 2
                onTriggered: App.addToLibrary(contextMenu.index, 2)
            }

            Action {
                text: "Dropped"
                enabled: contextMenu.libraryType !== 3
                onTriggered: App.addToLibrary(contextMenu.index, 3)
            }

            Action {
                text: "Completed"
                enabled: contextMenu.libraryType !== 4
                onTriggered: App.addToLibrary(contextMenu.index, 4)
            }
        }

        Action {
            id: removeMenuItem
            text: "Remove From Library"
            enabled: contextMenu.libraryType !== -1
            onTriggered: App.library.remove(contextMenu.link)
        }
    }

    Keys.enabled: true
    Keys.onPressed: event => {
        if (event.modifiers & Qt.ControlModifier) {
            if (event.key === Qt.Key_R) App.explorer.reload()
        } else {
            switch (event.key) {
                case Qt.Key_Escape:
                case Qt.Key_Alt:
                if (searchTextField.activeFocus) explorerPage.forceActiveFocus()
                break

                case Qt.Key_Tab:
                providerComboBox.popup.close()
                App.providerManager.cycleProviders()
                event.accepted = true
                break

                case Qt.Key_Enter:
                search()
                break

                case Qt.Key_Slash:
                searchTextField.forceActiveFocus()
                break

                case Qt.Key_P:
                App.explore("", 1, false)
                break

                case Qt.Key_L:
                App.explore("", 1, true)
                break

                case Qt.Key_Up:
                gridView.flick(0, 500)
                break

                case Qt.Key_Down:
                gridView.flick(0, -500)
                break
            }
        }
    }
}
