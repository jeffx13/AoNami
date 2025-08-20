pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import Kyokou.App.Main

Rectangle {
    id: libraryPage
    color: "#0B1220"
    
    property var swipeView

    LoadingScreen {
        id: loadingScreen
        anchors.centerIn: parent
        z: parent.z + 1
        loading: App.showManager.isLoading && libraryPage.visible
    }

    Component.onDestruction: root.libraryLastContentY = libraryGridView.contentY

    Keys.onPressed: (event) => {
        if (event.modifiers & Qt.ControlModifier) {
            if (event.key === Qt.Key_R) {
                App.library.fetchUnwatchedEpisodes(App.library.libraryType)
            }
        } else {
            if (event.key === Qt.Key_Tab) {
                event.accepted = true
                libraryTypeComboBox.popup.close()
                App.library.cycleDisplayLibraryType()
                libraryTypeComboBox.currentIndex = App.library.libraryType
            }
        }
    }

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
                id: libraryTypeComboBox
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1.5
                fontSize: 20
                text: "text"
                currentIndex: App.library.libraryType
                focusPolicy: Qt.NoFocus
                
                onActivated: (index) => {
                    App.library.libraryType = index
                }
                
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
                fontSize: 20
                text: "text"
                currentIndex: App.libraryModel.typeFilter
                focusPolicy: Qt.NoFocus
                
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
                    text: App.libraryModel.titleFilter
                    focusPolicy: Qt.NoFocus
                    anchors.fill: parent
                    
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
                id: hasUnwatchedEpisodesOnlyCheckBox
                text: qsTr("Unwatched")
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1.5
                font.pixelSize: 20 * root.fontSizeMultiplier
                focusPolicy: Qt.NoFocus
                
                Component.onCompleted: checked = App.libraryModel.hasUnwatchedEpisodesOnly
                onClicked: App.libraryModel.hasUnwatchedEpisodesOnly = checked
                
                indicator: Rectangle {
                    implicitWidth: 22
                    implicitHeight: 22
                    x: hasUnwatchedEpisodesOnlyCheckBox.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 4
                    border.color: hasUnwatchedEpisodesOnlyCheckBox.checked ? "#4E5BF2" : "#334E5BF2"
                    color: "transparent"
                    
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
                    anchors.left: parent.indicator.right
                    anchors.leftMargin: 6
                }
            }

            Text {
                text: `${libraryGridView.count} Show(s)`
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

    // https://doc.qt.io/qt-6/qtquick-tutorials-dynamicview-dynamicview3-example.html
    // https://www.reddit.com/r/QtFramework/comments/1f1oikv/qml_drag_and_drop_with_gridview/
    MediaGridView {
        id: libraryGridView
        focusPolicy: Qt.NoFocus
        
        property real upperBoundary: 0.1 * libraryGridView.height
        property real lowerBoundary: 0.9 * libraryGridView.height

        signal dragFinished(int from, int to)
        signal contextMenuRequested(int index)
        
        anchors {
            left: parent.left
            top: topBarCard.bottom
            bottom: parent.bottom
            right: parent.right
            rightMargin: 20
        }
        
        onDragFinished: (from, to) => {
            let lastContentY = libraryGridView.contentY
            App.library.move(from, to) // Resets contentY to 0
            libraryGridView.contentY = lastContentY
        }
        
        onContextMenuRequested: (index) => {
            contextMenu.index = index
            contextMenu.popup()
        }
        
        Component.onCompleted: {
            contentY = root.libraryLastContentY
            forceActiveFocus()
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
            
            contentItem: Rectangle {
                color: "#2F3B56"
                radius: width / 2
            }
            
            background: Rectangle {
                color: "#121826"
                radius: width / 2
            }
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

                ShowItem {
                    id: dragBox
                    showTitle: title
                    showCover: cover
                    width: dropCell.width
                    height: dropCell.height
                    
                    property int _index: index
                    
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    
                    Drag.active: dragHandle.drag.active
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
                    
                    onImageClicked: (mouse) => {
                        if (mouse.button === Qt.LeftButton) {
                            App.loadShow(App.libraryModel.mapToAbsoluteIndex(index), true)
                        } else if (mouse.button === Qt.RightButton) {
                            libraryGridView.contextMenuRequested(App.libraryModel.mapToAbsoluteIndex(index))
                        } else if (mouse.button === Qt.MiddleButton) {
                            App.appendToPlaylists(App.libraryModel.mapToAbsoluteIndex(index), true, false)
                        }
                    }

                    Rectangle {
                        width: 40
                        height: width
                        radius: width / 2
                        color: "white"
                        border.color: "red"
                        border.width: 5
                        visible: unwatchedEpisodes > 0
                        
                        anchors {
                            top: parent.top
                            right: parent.right
                        }
                        
                        Text {
                            text: unwatchedEpisodes
                            color: "black"
                            anchors.centerIn: parent
                        }
                    }

                    MouseArea {
                        id: dragHandle
                        // Can't anchor to image because it's not a parent or sibling
                        x: dragBox.image.x
                        y: dragBox.image.y
                        width: dragBox.image.width
                        height: dragBox.image.height
                        propagateComposedEvents: true
                        drag.target: dragBox
                        cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.PointingHandCursor
                        
                        onReleased: dragBox.Drag.drop()
                        
                        onMouseYChanged: (mouse) => {
                            if (!drag.active) return
                            let relativeY = libraryGridView.mapFromItem(dragHandle, mouse.x, mouse.y).y
                            if (!libraryGridView.atYBeginning && relativeY < libraryGridView.upperBoundary) {
                                libraryGridView.contentY -= 6
                            } else if (!libraryGridView.atYEnd && relativeY > libraryGridView.lowerBoundary) {
                                libraryGridView.contentY += 6
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
                                parent: libraryGridView
                            }
                        }
                    ]
                }
                
                onDropped: function (drop) {
                    let from = drop.source._index
                    let to = index
                    if (from === to) return
                    
                    libraryGridView.dragFinished(
                        App.libraryModel.mapToAbsoluteIndex(from), 
                        App.libraryModel.mapToAbsoluteIndex(to)
                    )
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
            text: "Append To Playlists"
            onTriggered: App.appendToPlaylists(contextMenu.index, true, false)
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
            text: "Remove From library"
            onTriggered: App.library.removeAt(contextMenu.index)
        }
    }
}
