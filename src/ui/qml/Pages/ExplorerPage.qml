pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import AoNami
import ".."

Item {
    id: explorerPage
    focus: true

    // Browse mode for the toolbar's active-state highlight: 0 Latest, 1 Popular, 2 Search.
    property int browseMode: 0

    function search() {
        let q = searchTextField.text.trim()
        if (q.length > 0)
            App.settings.prependToHistory("search/history", q)
        historyPopup.close()
        explorerPage.forceActiveFocus()
        browseMode = q.length > 0 ? 2 : 1
        App.explore(searchTextField.text, 1, false)
    }

    HoverHandler {
        cursorShape: Qt.ArrowCursor
    }

    Component.onDestruction: {
        Globals.lastSearch = searchTextField.text
        Globals.explorerLastContentY = gridView.contentY
    }

    Popup {
        id: historyPopup
        parent: Overlay.overlay
        modal: false
        padding: 6
        margins: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property var historyItems: []

        background: Rectangle {
            color: Theme.surface
            radius: 10
            border.color: Theme.border
            border.width: 1
        }

        enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 120; easing.type: Easing.OutCubic } }
        exit:  Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 90;  easing.type: Easing.InCubic  } }

        contentItem: Column {
            spacing: 0

            ListView {
                id: historyList
                model: historyPopup.historyItems
                width: parent.width
                height: implicitHeight
                implicitHeight: Math.min(count * 36, 252)
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                spacing: 2

                delegate: AbstractButton {
                    id: historyBtn
                    required property string modelData
                    width: historyList.width
                    height: 34
                    focusPolicy: Qt.NoFocus

                    background: Rectangle {
                        radius: 6
                        color: historyBtn.hovered ? Theme.border : "transparent"
                        Behavior on color { ColorAnimation { duration: 80 } }
                    }

                    contentItem: RowLayout {
                        anchors { fill: parent; leftMargin: 10; rightMargin: 4 }
                        spacing: 8
                        AppIcon {
                            name: "search"
                            size: 16
                            color: Theme.textMuted
                        }
                        Text {
                            Layout.fillWidth: true
                            text: historyBtn.modelData
                            color: Theme.textSecondary
                            font.pixelSize: Globals.sp(20)
                            elide: Text.ElideRight
                        }
                        AppIcon {
                            name: "arrow-up-left"
                            size: 16
                            color: historyBtn.hovered ? Theme.accent : Theme.textMuted
                            Behavior on color { ColorAnimation { duration: 80 } }
                        }
                        AbstractButton {
                            id: deleteBtn
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            focusPolicy: Qt.NoFocus

                            background: Rectangle {
                                radius: 4
                                color: deleteBtn.hovered ? Qt.alpha(Theme.accent, 0.12) : "transparent"
                                Behavior on color { ColorAnimation { duration: 80 } }
                            }

                            contentItem: Item {
                                AppIcon {
                                    anchors.centerIn: parent
                                    name: "x"
                                    size: 13
                                    color: deleteBtn.hovered ? Theme.textPrimary : Theme.textMuted
                                }
                            }

                            onClicked: {
                                App.settings.removeFromHistory("search/history", historyBtn.modelData)
                                let h = App.settings.getStringList("search/history")
                                if (h.length === 0) {
                                    historyPopup.close()
                                } else {
                                    historyPopup.historyItems = h
                                }
                            }
                        }
                    }

                    onClicked: {
                        searchTextField.text = historyBtn.modelData
                        historyPopup.close()
                        explorerPage.search()
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.border
                visible: historyPopup.historyItems.length > 0
            }

            AbstractButton {
                id: clearAllBtn
                width: parent.width
                height: 32
                focusPolicy: Qt.NoFocus

                background: Rectangle {
                    radius: 6
                    color: clearAllBtn.hovered ? Theme.border : "transparent"
                    Behavior on color { ColorAnimation { duration: 80 } }
                }

                contentItem: Text {
                    text: "Clear History"
                    color: clearAllBtn.hovered ? Theme.danger : Theme.textMuted
                    font.pixelSize: Globals.sp(16)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 80 } }
                }

                onClicked: {
                    App.settings.clearHistory("search/history")
                    historyPopup.close()
                }
            }
        }
    }

    Rectangle {
        id: searchBarCard
        height: Math.max(48, parent.height * 0.065)
        radius: 14
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: 10
            leftMargin: 8
            rightMargin: 8
        }

        RowLayout {
            anchors {
                fill: parent
                margins: 6
            }
            spacing: 6

            AppTextField {
                id: searchTextField
                color: Theme.textPrimary
                placeholderText: qsTr("Enter query!")
                placeholderTextColor: Theme.textMuted
                text: Globals.lastSearch
                fontSize: 20
                showClearButton: true
                focusPolicy: Qt.NoFocus
                focus: false
                activeFocusOnTab: false
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 5
                onAccepted: explorerPage.search()
                onActiveFocusChanged: {
                    if (activeFocus) {
                        let h = App.settings.getStringList("search/history")
                        if (h.length > 0) {
                            historyPopup.historyItems = h
                            let p = searchTextField.mapToItem(Overlay.overlay, 0, searchTextField.height)
                            historyPopup.x = p.x
                            historyPopup.y = p.y + 4
                            historyPopup.width = searchTextField.width
                            historyPopup.open()
                        }
                    } else {
                        historyPopup.close()
                    }
                }
            }

            AppButton {
                text: "Search"
                fontSize: 20
                radius: 10
                focusPolicy: Qt.NoFocus
                focus: false
                activeFocusOnTab: false
                Layout.fillHeight: true
                leftPadding: 16
                rightPadding: 16
                onClicked: explorerPage.search()
            }

            AppButton {
                text: "Latest"
                radius: 10
                backgroundDefaultColor: explorerPage.browseMode === 0 ? Theme.accent : Theme.border
                focusPolicy: Qt.NoFocus
                focus: false
                activeFocusOnTab: false
                Layout.fillHeight: true
                leftPadding: 16
                rightPadding: 16
                onClicked: { explorerPage.forceActiveFocus(); explorerPage.browseMode = 0; App.explore("", 1, true) }
            }

            AppButton {
                text: "Popular"
                radius: 10
                backgroundDefaultColor: explorerPage.browseMode === 1 ? Theme.accent : Theme.border
                focusPolicy: Qt.NoFocus
                focus: false
                activeFocusOnTab: false
                Layout.fillHeight: true
                leftPadding: 16
                rightPadding: 16
                onClicked: { explorerPage.forceActiveFocus(); explorerPage.browseMode = 1; App.explore("", 1, false) }
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
                text: ""
                fontSize: 20
                focus: false
                model: App.providerManager.availableShowTypes
                currentIndex: App.providerManager.currentSearchTypeIndex
                currentIndexColor: Qt.alpha(Theme.accent, 0.25)
                activeFocusOnTab: false
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 2
                onActivated: (index) => {
                    App.providerManager.currentSearchTypeIndex = index
                }
            }
        }
    }

    MediaGridView {
        id: gridView
        model: App.explorer
        focus: false
        anchors {
            top: searchBarCard.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            rightMargin: 20
            topMargin: 6
        }
        imageAspectRatio: Globals.imageAspectRatio
        Component.onCompleted: contentY = Globals.explorerLastContentY

        readonly property real fetchThreshold: height * 1.0

        function tryFetchMore() {
            if (!App.explorer.canFetchMore()) return
            if (gridView.height <= 0) return

            let distanceFromBottom = contentHeight - (contentY + height)
            if (contentHeight <= height || distanceFromBottom < fetchThreshold) {
                App.explorer.fetchMore()
            }
        }

        onContentYChanged: tryFetchMore()
        onContentHeightChanged: tryFetchMore()
        onHeightChanged: tryFetchMore()

        Connections {
            target: App.explorer
            function onIsLoadingChanged() {
                if (!App.explorer.isLoading) {
                    fetchDebounce.restart()
                }
            }
        }

        Timer {
            id: fetchDebounce
            interval: 50
            repeat: false
            onTriggered: gridView.tryFetchMore()
        }

        ScrollBar.vertical: AppScrollBar {
            parent: gridView.parent
            width: 6
            barColor: Theme.accent
            barOpacity: 0.4
            showTrack: true
            anchors {
                top: gridView.top
                left: gridView.right
                bottom: gridView.bottom
            }
        }

        onImageAspectRatioChanged: {
            let lastContentY = contentY
            App.explorer.reset()
            contentY = lastContentY
        }

        delegate: ShowItem {
            required property string title
            required property string link
            required property string cover
            required property string latestTxt
            required property int index

            showTitle: title
            showCover: cover
            badgeText: latestTxt
            width: gridView.cellWidth
            height: gridView.cellHeight
            aspectRatio: Globals.imageAspectRatio

            property int _libRev: 0
            libraryType: { _libRev; return App.library.getLibraryType(link) }

            Connections {
                target: App.library
                function onLibraryChanged() { _libRev++ }
                function onModelReset()     { _libRev++ }
            }

            onImageLoaded: (sourceAspectRatio) => {
                if (index !== 0) return
                if (Math.abs(Globals.imageAspectRatio - sourceAspectRatio) < 0.01) return
                Globals.imageAspectRatio = sourceAspectRatio
            }

            onImageClicked: (mouse) => {
                explorerPage.forceActiveFocus()
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

            onPlayClicked: {
                explorerPage.forceActiveFocus()
                App.appendToPlaylists(index, false, true)
            }
            onAddClicked: {
                explorerPage.forceActiveFocus()
                contextMenu.index = index
                contextMenu.libraryType = App.library.getLibraryType(link)
                contextMenu.link = link
                contextMenu.popup()
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
            text: "Queue"
            onTriggered: App.appendToPlaylists(contextMenu.index, false, false)
        }

        AppMenu {
            id: librarySubMenu
            title: contextMenu.libraryType === -1 ? "Add To Library" : "Change Library Type"

            Instantiator {
                model: Globals.libraryTypes
                delegate: Action {
                    required property int    index
                    required property string modelData
                    text: modelData
                }
                onObjectAdded: (i, obj) => {
                    obj.enabled = Qt.binding(() => contextMenu.libraryType !== obj.index)
                    obj.triggered.connect(() => App.addToLibrary(contextMenu.index, obj.index))
                    librarySubMenu.insertAction(i, obj)
                }
                onObjectRemoved: (i, obj) => librarySubMenu.removeAction(obj)
            }
        }

        Action {
            text: "Remove From Library"
            enabled: contextMenu.libraryType !== -1
            onTriggered: App.library.remove(contextMenu.link)
        }
    }

    TapHandler {
        onTapped: explorerPage.forceActiveFocus()
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
                case Qt.Key_Return:
                if (gridView.currentIndex >= 0 && !searchTextField.activeFocus) {
                    App.loadShow(gridView.currentIndex, false)
                    event.accepted = true
                } else {
                    search()
                }
                break

                case Qt.Key_Slash:
                searchTextField.forceActiveFocus()
                event.accepted = true
                break

                case Qt.Key_P:
                explorerPage.browseMode = 1
                App.explore("", 1, false)
                break

                case Qt.Key_L:
                explorerPage.browseMode = 0
                App.explore("", 1, true)
                break

                case Qt.Key_Left:
                if (gridView.count > 0)
                    gridView.currentIndex = gridView.currentIndex <= 0 ? 0 : gridView.currentIndex - 1
                event.accepted = true
                break

                case Qt.Key_Right:
                if (gridView.count > 0)
                    gridView.currentIndex = gridView.currentIndex < 0 ? 0 : Math.min(gridView.count - 1, gridView.currentIndex + 1)
                event.accepted = true
                break

                case Qt.Key_Up:
                if (gridView.count > 0)
                    gridView.currentIndex = gridView.currentIndex < 0 ? 0 : Math.max(0, gridView.currentIndex - gridView.itemPerRow)
                event.accepted = true
                break

                case Qt.Key_Down:
                if (gridView.count > 0)
                    gridView.currentIndex = gridView.currentIndex < 0 ? 0 : Math.min(gridView.count - 1, gridView.currentIndex + gridView.itemPerRow)
                event.accepted = true
                break
            }
        }
    }
}
