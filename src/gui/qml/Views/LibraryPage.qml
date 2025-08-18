import QtQuick
import QtQuick.Controls
import "../Components"
import QtQuick.Layouts
import Kyokou.App.Main

Rectangle {
    id: libraryPage
    property var swipeView
    color: "#0B1220"

    LoadingScreen {
        id:loadingScreen
        anchors.centerIn: parent
        z: parent.z + 1
        loading: App.showManager.isLoading && libraryPage.visible
    }

    Keys.onPressed: (event) => {
                        if (event.modifiers & Qt.ControlModifier) {
                            if (event.key === Qt.Key_R) {
                                App.library.fetchUnwatchedEpisodes(App.library.libraryType)
                            }
                        } else {
                            if (event.key === Qt.Key_Tab) {
                                event.accepted = true
                                libraryTypeComboBox.popup.close()
                                App.library.cycleDisplayLibraryType() ;
                                libraryTypeComboBox.currentIndex = App.library.libraryType
                            }
                        }
                    }

    // Component.onCompleted: App.libraryModel.

    Rectangle {
        id: topBarCard
        height: Math.max(56, parent.height * 0.08)
        radius: 12
        color: "#0f1324cc"
        border.color: "#334E5BF2"
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
                id:libraryTypeComboBox
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1.5
                fontSize: 20
                text: "text"
                currentIndex: App.library.libraryType
                onActivated: (index) => {App.library.libraryType = index}
                model: ListModel{
                    ListElement { text: "Watching" }
                    ListElement { text: "Planned" }
                    ListElement { text: "Paused" }
                    ListElement { text: "Dropped" }
                    ListElement { text: "Completed" }
                }
                focusPolicy: Qt.NoFocus
            }
            AppComboBox {
                id: showTypeComboBox
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1.5
                fontSize: 20
                text: "text"
                currentIndex: App.libraryModel.typeFilter
                focusPolicy: Qt.NoFocus
                onActivated: (index) => App.libraryModel.typeFilter = index

                model: ListModel{
                    ListElement { text: "All"}
                    ListElement { text: "Animes"}
                    ListElement { text: "Movies"}
                    ListElement { text: "Tv Series"}
                    ListElement { text: "Variety Shows"}
                    ListElement { text: "Documentaries"}
                }
            }

            Item {
                id: searchContainer
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 4.5
                
                AppTextField {
                    id: titleFilterTextField
                    checkedColor: "#727CF5"
                    color: "white"
                    placeholderText: qsTr("Search")
                    placeholderTextColor: "gray"
                    fontSize: 20
                    anchors.fill: parent
                    text: App.libraryModel.titleFilter
                    focusPolicy: Qt.NoFocus
                    Binding {
                        target: App.libraryModel
                        property: "titleFilter"
                        value: titleFilterTextField.text
                    }

                }
                
                Text {
                    id: regexToggle
                    text: "(.*)"
                    font.pixelSize: 16 * root.fontSizeMultiplier
                    color: App.libraryModel.useRegex ? "#4E5BF2" : "#6b7280"
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
                        onEntered: parent.color = App.libraryModel.useRegex ? "#4E5BF2" : "#9ca3af"
                        onExited: parent.color = App.libraryModel.useRegex ? "#4E5BF2" : "#6b7280"
                    }
                }
                
                Text {
                    id: caseToggle
                    text: "Aa"
                    font.pixelSize: 16 * root.fontSizeMultiplier
                    color: App.libraryModel.caseSensitive ? "#4E5BF2" : "#6b7280"
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
                        onEntered: parent.color = App.libraryModel.caseSensitive ? "#4E5BF2" : "#9ca3af"
                        onExited: parent.color = App.libraryModel.caseSensitive ? "#4E5BF2" : "#6b7280"
                    }
                }
            }
            
            CheckBox {  
                onClicked: App.libraryModel.hasUnwatchedEpisodesOnly = checked
                Component.onCompleted: checked = App.libraryModel.hasUnwatchedEpisodesOnly
                text: qsTr("Unwatched")
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1.5
                font.pixelSize: 20 * root.fontSizeMultiplier
                focusPolicy: Qt.NoFocus
                id: hasUnwatchedEpisodesOnlyCheckBox
                indicator: Rectangle {
                    implicitWidth: 22
                    implicitHeight: 22
                    x: hasUnwatchedEpisodesOnlyCheckBox.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 4
                    border.color: hasUnwatchedEpisodesOnlyCheckBox.checked ? "#4E5BF2" : "#334E5BF2"
                    color: "transparent"
                    id:indicator
                    Text {
                        width: 14
                        height: 14
                        x: 2
                        y: -1
                        text: "✔"
                        font.pointSize: 14
                        color: "#4E5BF2"
                        visible: hasUnwatchedEpisodesOnlyCheckBox.checked
                    }
                }
                contentItem: Text {
                    text: hasUnwatchedEpisodesOnlyCheckBox.text
                    font.pixelSize: 20 * root.fontSizeMultiplier
                    color: "#e8ebf6"
                    opacity: hasUnwatchedEpisodesOnlyCheckBox.enabled ? 1.0 : 0.3
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    anchors.left: indicator.right
                    anchors.leftMargin: 6

                }

            }

            Text {
                text:`${libraryGridView.count} Show(s)`
                font.pixelSize: 20 * root.fontSizeMultiplier
                color: "#e8ebf6"
                verticalAlignment: Qt.AlignVCenter
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignRight
            }
        }
    }

    MediaGridView {
        id: libraryGridView
        onContentYChanged: libraryLastScrollY = libraryGridView.contentY

        focusPolicy: Qt.NoFocus
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            parent: libraryGridView.parent
            anchors.top: libraryGridView.top
            anchors.left: libraryGridView.right
            anchors.bottom: libraryGridView.bottom
            width: 20
            contentItem: Rectangle {
                radius: width / 2
            }
            background: Rectangle {
                radius: width / 2
                color: 'transparent'
            }
        }

        onDragFinished: (from, to) => {
                            let lastContentY = libraryGridView.contentY
                            App.library.move(from, to); // Resets contentY to 0
                            libraryGridView.contentY = lastContentY
                        }
        onContextMenuRequested: (index) =>{
                                    contextMenu.index = index
                                    contextMenu.popup()
                                }

        anchors {
            left: parent.left
            top: topBarCard.bottom
            bottom: parent.bottom
            right: parent.right
            rightMargin: 20
        }
        Component.onCompleted: {
            contentY = root.libraryLastScrollY
            forceActiveFocus()
            App.libraryModel.refresh()
        }

        property real upperBoundary: 0.1 * libraryGridView.height
        property real lowerBoundary: 0.9 * libraryGridView.height



        signal dragFinished(int from, int to)
        signal contextMenuRequested(int index)

        // https://doc.qt.io/qt-6/qtquick-tutorials-dynamicview-dynamicview3-example.html
        // https://www.reddit.com/r/QtFramework/comments/1f1oikv/qml_drag_and_drop_with_gridview/
        model: DelegateModel {
            id: visualModel
            model: App.libraryModel
            delegate: DropArea {
                id: dropCell
                width: libraryGridView.cellWidth
                height: libraryGridView.cellHeight


                ShowItem {
                    id: dragBox
                    showTitle: model.title
                    showCover: model.cover
                    property int _index: model.index
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    onImageClicked: (mouse) => {
                                        if (mouse.button === Qt.LeftButton) {
                                            App.loadShow(App.libraryModel.mapToAbsoluteIndex(model.index), true)
                                        } else if (mouse.button === Qt.RightButton){
                                            libraryGridView.contextMenuRequested(App.libraryModel.mapToAbsoluteIndex(model.index))
                                        } else if (mouse.button === Qt.MiddleButton) {
                                            App.appendToPlaylists(App.libraryModel.mapToAbsoluteIndex(model.index), true, false)
                                        }
                                    }
                    Drag.active: dragHandle.drag.active
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
                    width: dropCell.width; height: dropCell.height

                    Rectangle {
                        anchors {
                            top: parent.top
                            right:parent.right
                        }
                        width: 40
                        height: width
                        radius: width / 2
                        color: "white"
                        border.color: "red"
                        border.width: 5
                        Text {
                            text: model.unwatchedEpisodes
                            color: "black"
                            anchors.centerIn: parent
                        }
                        visible: model.unwatchedEpisodes > 0
                    }

                    states: [
                        State {
                            when: dragBox.Drag.active

                            // disable anchors to allow dragBox to move
                            AnchorChanges {
                                target: dragBox
                                anchors.horizontalCenter: undefined
                                anchors.verticalCenter: undefined
                            }

                            // keep dragBox in front of other cells when dragging
                            ParentChange {
                                target: dragBox
                                parent: libraryGridView
                            }
                        }
                    ]

                    MouseArea {
                        id: dragHandle
                        propagateComposedEvents: true
                        drag.target: dragBox
                        onReleased: dragBox.Drag.drop()

                        anchors.fill: parent
                        onMouseYChanged: (mouse) => {
                                             if (!drag.active) return
                                             let relativeY = libraryGridView.mapFromItem(dragHandle, mouse.x, mouse.y).y
                                             if (!libraryGridView.atYBeginning && relativeY < libraryGridView.upperBoundary) {
                                                 libraryGridView.contentY -= 6
                                             } else if (!libraryGridView.atYEnd && relativeY > libraryGridView.lowerBoundary) {
                                                 libraryGridView.contentY += 6
                                             }
                                         }
                        cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.PointingHandCursor

                    }
                }
                onDropped: function (drop) {
                    let from = drop.source._index
                    let to = model.index
                    if (from === to) return
                    libraryGridView.dragFinished(App.libraryModel.mapToAbsoluteIndex(from), App.libraryModel.mapToAbsoluteIndex(to))
                }

                // onEntered: function (drag) {
                //     print("target:", drag.source._index, "into", model.index)
                //     visualModel.items.move(drag.source._index, model.index)
                // }

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




    Menu {
        id: contextMenu
        modal: true
        property int index

        Menu {
            title: "Change list type"
            MenuItem {
                visible: libraryTypeComboBox.currentIndex !== 0
                text: "Watching"
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 0, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: libraryTypeComboBox.currentIndex !== 1
                text: "Planned"
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 1, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: libraryTypeComboBox.currentIndex !== 2
                text: "Paused"
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 2, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: libraryTypeComboBox.currentIndex !== 3
                text: "Dropped"
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 3, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: libraryTypeComboBox.currentIndex !== 4
                text: "Completed"
                onTriggered: App.library.changeLibraryTypeAt(contextMenu.index, 4, -1)
                height: visible ? implicitHeight : 0
            }

        }

        MenuItem {
            text: "Play"
            onTriggered:  {
                App.appendToPlaylists(contextMenu.index, true, true)
            }
        }
        MenuItem {
            text: "Append to Playlists"
            onTriggered:  {
                App.appendToPlaylists(contextMenu.index, true, false)
            }
        }

        MenuItem {
            text: "Remove from library"
            onTriggered:  {
                App.library.removeAt(contextMenu.index)
            }
        }
    }

}
