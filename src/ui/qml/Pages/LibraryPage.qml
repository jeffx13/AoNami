pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import AoNami
import ".."

Rectangle {
    id: libraryPage
    color: "transparent"

    LoadingScreen {
        id: loadingScreen
        anchors.centerIn: parent
        z: parent.z + 1
        loading: App.showManager.isLoading && libraryPage.visible
    }

    HoverHandler {
        cursorShape: Qt.ArrowCursor
    }

    Keys.onPressed: (event) => {
        if (event.modifiers & Qt.ControlModifier) {
            if (event.key === Qt.Key_R)
                App.library.fetchUnwatchedEpisodes(App.library.libraryType, true)
            return
        }
        switch (event.key) {
        case Qt.Key_Tab:
            event.accepted = true
            libraryTypeComboBox.popup.close()
            App.library.cycleDisplayLibraryType()
            libraryTypeComboBox.currentIndex = App.library.libraryType
            break
        case Qt.Key_Left:
            if (libraryGridView.count > 0)
                libraryGridView.currentIndex = libraryGridView.currentIndex <= 0 ? 0 : libraryGridView.currentIndex - 1
            event.accepted = true
            break
        case Qt.Key_Right:
            if (libraryGridView.count > 0)
                libraryGridView.currentIndex = libraryGridView.currentIndex < 0 ? 0 : Math.min(libraryGridView.count - 1, libraryGridView.currentIndex + 1)
            event.accepted = true
            break
        case Qt.Key_Up:
            if (libraryGridView.count > 0)
                libraryGridView.currentIndex = libraryGridView.currentIndex < 0 ? 0 : Math.max(0, libraryGridView.currentIndex - libraryGridView.itemPerRow)
            event.accepted = true
            break
        case Qt.Key_Down:
            if (libraryGridView.count > 0)
                libraryGridView.currentIndex = libraryGridView.currentIndex < 0 ? 0 : Math.min(libraryGridView.count - 1, libraryGridView.currentIndex + libraryGridView.itemPerRow)
            event.accepted = true
            break
        case Qt.Key_Enter:
        case Qt.Key_Return:
            if (libraryGridView.currentIndex >= 0) {
                App.loadShow(App.libraryModel.mapToAbsoluteIndex(libraryGridView.currentIndex), true)
                event.accepted = true
            }
            break
        }
    }

    Rectangle {
        id: topBarCard
        height: Math.max(56, parent.height * 0.08)
        radius: 12
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 12
            leftMargin: 12
            rightMargin: 12
        }

        RowLayout {
            id: topBar
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            AppComboBox {
                id: libraryTypeComboBox
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1.5
                focusPolicy: Qt.NoFocus
                currentIndex: App.library.libraryType
                onActivated: (index) => App.library.libraryType = index
                model: ListModel {
                    ListElement { text: "Watching" }
                    ListElement { text: "Planned" }
                    ListElement { text: "Paused" }
                    ListElement { text: "Dropped" }
                    ListElement { text: "Completed" }
                }
            }

            AppComboBox {
                id: showTypeComboBox
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1.5
                focusPolicy: Qt.NoFocus
                currentIndex: App.libraryModel.typeFilter
                onActivated: (index) => App.libraryModel.typeFilter = index
                model: ListModel {
                    ListElement { text: "All" }
                    ListElement { text: "Animes" }
                    ListElement { text: "Movies" }
                    ListElement { text: "Tv Series" }
                    ListElement { text: "Variety Shows" }
                    ListElement { text: "Documentaries" }
                }
            }

            AppComboBox {
                id: sortComboBox
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1.5
                focusPolicy: Qt.NoFocus
                currentIndex: App.libraryModel.sortRole
                onActivated: (index) => App.libraryModel.sortRole = index
                model: ListModel {
                    ListElement { text: "Default" }
                    ListElement { text: "A-Z" }
                    ListElement { text: "Unwatched" }
                }
            }

            Item {
                id: searchContainer
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 3.2

                AppTextField {
                    id: titleFilterTextField
                    anchors.fill: parent
                    checkedColor: Theme.accent
                    color: Theme.textPrimary
                    placeholderText: qsTr("Search")
                    placeholderTextColor: "gray"
                    focusPolicy: Qt.NoFocus
                    text: App.libraryModel.titleFilter
                    Binding {
                        target: App.libraryModel
                        property: "titleFilter"
                        value: titleFilterTextField.text
                    }
                }

                Text {
                    id: regexToggle
                    text: "(.*)"
                    font.pixelSize: Globals.sp(20)
                    color: App.libraryModel.useRegex ? Theme.accent : Theme.textMuted
                    anchors {
                        right: caseToggle.left
                        verticalCenter: parent.verticalCenter
                        rightMargin: 8
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.libraryModel.useRegex = !App.libraryModel.useRegex
                        onEntered: parent.color = App.libraryModel.useRegex ? Theme.accent : Theme.textMuted
                        onExited: parent.color = App.libraryModel.useRegex ? Theme.accent : Theme.textMuted
                    }
                }

                Text {
                    id: caseToggle
                    text: "Aa"
                    font.pixelSize: Globals.sp(20)
                    color: App.libraryModel.caseSensitive ? Theme.accent : Theme.textMuted
                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        rightMargin: 12
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.libraryModel.caseSensitive = !App.libraryModel.caseSensitive
                        onEntered: parent.color = App.libraryModel.caseSensitive ? Theme.accent : Theme.textMuted
                        onExited: parent.color = App.libraryModel.caseSensitive ? Theme.accent : Theme.textMuted
                    }
                }
            }

            Row {
                Layout.fillHeight: true
                Layout.preferredWidth: implicitWidth
                spacing: 6
                layoutDirection: Qt.RightToLeft
                AppCheckBox {
                    anchors.verticalCenter: parent.verticalCenter
                    focusPolicy: Qt.NoFocus
                    checked: App.libraryModel.hasUnwatchedEpisodesOnly
                    onClicked: App.libraryModel.hasUnwatchedEpisodesOnly = checked
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Unwatched")
                    font.pixelSize: Globals.sp(20)
                    color: Theme.textPrimary
                }
            }

            Text {
                text: libraryGridView.count + " Show(s)"
                font.pixelSize: Globals.sp(20)
                color: Theme.textSecondary
                horizontalAlignment: Qt.AlignRight
                verticalAlignment: Qt.AlignVCenter
                elide: Text.ElideNone
                Layout.fillHeight: true
                Layout.preferredWidth: implicitWidth
                Layout.rightMargin: 8
                Layout.alignment: Qt.AlignRight
            }
        }
    }

    Timer {
        id: autoScrollTimer
        interval: 16  // ~60fps
        repeat: true
        property int direction: 0  // -1 = up, +1 = down, 0 = stop

        onTriggered: {
            if (direction < 0 && !libraryGridView.atYBeginning)
                libraryGridView.contentY = Math.max(0, libraryGridView.contentY - 8)
            else if (direction > 0 && !libraryGridView.atYEnd)
                libraryGridView.contentY = Math.min(libraryGridView.contentHeight - libraryGridView.height, libraryGridView.contentY + 8)
        }
    }

    MediaGridView {
        id: libraryGridView
        focusPolicy: Qt.NoFocus

        signal contextMenuRequested(int index)

        anchors {
            left: parent.left
            top: topBarCard.bottom
            bottom: parent.bottom
            right: parent.right
            rightMargin: 20
        }

        Component.onDestruction: Globals.libraryLastContentY = contentY
        Component.onCompleted: contentY = Globals.libraryLastContentY

        onContextMenuRequested: (index) => {
            contextMenu.index = index
            contextMenu.popup()
        }

        displaced: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 200
                easing.type: Easing.OutCubic
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            parent: libraryGridView.parent
            width: 8
            anchors {
                top: libraryGridView.top
                left: libraryGridView.right
                bottom: libraryGridView.bottom
            }
            contentItem: Rectangle { color: Theme.textMuted; radius: width / 2 }
            background: Rectangle { color: Theme.surfaceAlt; radius: width / 2 }
        }

        model: DelegateModel {
            id: visualModel
            model: App.libraryModel

            delegate: DropArea {
                id: dropCell
                width: libraryGridView.cellWidth
                height: libraryGridView.cellHeight
                required property string title
                required property string cover
                required property int index
                required property int unwatchedEpisodes

                property int visualIndex: DelegateModel.itemsIndex

                onEntered: function(drag) {
                    let from = drag.source.visualIndex
                    let to = dropCell.visualIndex
                    if (from !== to)
                        visualModel.items.move(from, to)
                }

                onDropped: function(drop) {
                    let fromAbsolute = drop.source.dragStartAbsoluteIndex
                    let toAbsolute = App.libraryModel.mapToAbsoluteIndex(dropCell.visualIndex)
                    if (fromAbsolute < 0 || fromAbsolute === toAbsolute) return

                    let y = libraryGridView.contentY
                    App.library.move(fromAbsolute, toAbsolute)
                    libraryGridView.contentY = y
                }

                ShowItem {
                    id: dragBox
                    showTitle: title
                    showCover: cover
                    width: dropCell.width
                    height: dropCell.height
                    showAddAction: false

                    property int visualIndex: dropCell.visualIndex
                    property int dragStartAbsoluteIndex: -1

                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter

                    Drag.active: dragHandle.drag.active
                    Drag.source: dragBox
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2

                    onImageClicked: (mouse) => {
                        if (mouse.button === Qt.LeftButton)
                            App.loadShow(App.libraryModel.mapToAbsoluteIndex(index), true)
                        else if (mouse.button === Qt.RightButton)
                            libraryGridView.contextMenuRequested(App.libraryModel.mapToAbsoluteIndex(index))
                        else if (mouse.button === Qt.MiddleButton)
                            App.appendToPlaylists(App.libraryModel.mapToAbsoluteIndex(index), true, false)
                    }

                    onPlayClicked: App.appendToPlaylists(App.libraryModel.mapToAbsoluteIndex(index), true, true)

                    Rectangle {
                        id: badgeBg
                        visible: unwatchedEpisodes > 0
                        width: Math.max(34, badgeText.implicitWidth + 14)
                        height: 34
                        radius: 17
                        anchors {
                            top: parent.top
                            right: parent.right
                            topMargin: 6
                            rightMargin: 6
                        }

                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#FF6B6B" }
                            GradientStop { position: 1.0; color: "#EE4444" }
                        }
                        border.color: "#CC3333"
                        border.width: 1

                        Text {
                            id: badgeText
                            anchors.centerIn: parent
                            text: unwatchedEpisodes
                            color: "white"
                            font {
                                pixelSize: Globals.sp(20)
                                bold: true
                            }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width + 6
                            height: parent.height + 6
                            radius: height / 2
                            color: "transparent"
                            border.color: "#40FF4444"
                            border.width: 2
                        }

                        Rectangle {
                            id: pulse
                            anchors.centerIn: parent
                            width: parent.width
                            height: parent.height
                            radius: height / 2
                            color: "transparent"
                            border.color: "#FF4444"
                            border.width: 2
                            opacity: 0

                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                NumberAnimation { to: 0.6; duration: 800; easing.type: Easing.OutCubic }
                                NumberAnimation { to: 0.0; duration: 800; easing.type: Easing.InCubic }
                            }
                            NumberAnimation on scale {
                                loops: Animation.Infinite
                                from: 1.0
                                to: 1.6
                                duration: 1600
                            }
                        }
                    }

                    MouseArea {
                        id: dragHandle
                        x: dragBox.image.x
                        y: dragBox.image.y
                        width: dragBox.image.width
                        height: dragBox.image.height
                        propagateComposedEvents: true
                        drag.target: App.libraryModel.sortRole !== 0 ? null : dragBox
                        cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.PointingHandCursor

                        onPressed: {
                            dragBox.dragStartAbsoluteIndex = App.libraryModel.mapToAbsoluteIndex(index)
                        }

                        onReleased: {
                            autoScrollTimer.direction = 0
                            autoScrollTimer.stop()
                            dragBox.Drag.drop()
                        }

                        onPositionChanged: (mouse) => {
                            if (!drag.active) return

                            let posInGrid = libraryGridView.mapFromItem(dragHandle, mouse.x, mouse.y)

                            if (posInGrid.y < libraryGridView.height * 0.1) {
                                autoScrollTimer.direction = -1
                                if (!autoScrollTimer.running) autoScrollTimer.start()
                            } else if (posInGrid.y > libraryGridView.height * 0.9) {
                                autoScrollTimer.direction = 1
                                if (!autoScrollTimer.running) autoScrollTimer.start()
                            } else {
                                autoScrollTimer.direction = 0
                                autoScrollTimer.stop()
                            }
                        }
                    }

                    states: [
                        State {
                            when: dragBox.Drag.active

                            AnchorChanges {
                                target: dragBox
                                anchors.horizontalCenter: undefined
                                anchors.verticalCenter: undefined
                            }

                            ParentChange {
                                target: dragBox
                                parent: libraryPage
                            }
                        }
                    ]
                }

                states: [
                    State {
                        when: dropCell.containsDrag && dropCell.drag.source != dragBox
                        PropertyChanges {
                            target: dropCell
                            opacity: 0.7
                        }
                    }
                ]
            }
        }
    }

    AppMenu {
        id: contextMenu
        modal: true
        property int index

        Action {
            text: "Play"
            onTriggered: App.appendToPlaylists(contextMenu.index, true, true)
        }

        Action {
            text: "Queue"
            onTriggered: App.appendToPlaylists(contextMenu.index, true, false)
        }

        AppMenu {
            title: "Move"
            Action {
                text: "Move to Top"
                onTriggered: {
                    App.library.move(contextMenu.index, 0)
                    libraryGridView.contentY = 0
                }
            }
            Action {
                text: "Move to Bottom"
                onTriggered: {
                    App.library.move(contextMenu.index, App.library.count() - 1)
                    libraryGridView.contentY = libraryGridView.contentHeight - libraryGridView.height
                }
            }
        }

        AppMenu {
            title: "Change Library Type"
            Action {
                text: "Watching"
                enabled: libraryTypeComboBox.currentIndex !== 0
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 0, -1)
            }
            Action {
                text: "Planned"
                enabled: libraryTypeComboBox.currentIndex !== 1
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 1, -1)
            }
            Action {
                text: "Paused"
                enabled: libraryTypeComboBox.currentIndex !== 2
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 2, -1)
            }
            Action {
                text: "Dropped"
                enabled: libraryTypeComboBox.currentIndex !== 3
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 3, -1)
            }
            Action {
                text: "Completed"
                enabled: libraryTypeComboBox.currentIndex !== 4
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 4, -1)
            }
        }

        Action {
            text: "Remove From Library"
            onTriggered: App.library.removeAt(contextMenu.index)
        }
    }
}