import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"
import QtQuick.Layouts 1.15
import Kyokou.App.Main
Rectangle{
    id: libraryPage
    property var swipeView
    color: "black"

    LoadingScreen {
        id:loadingScreen
        anchors.centerIn: parent
        z: parent.z + 1
        loading: App.currentShow.isLoading && libraryPage.visible
    }
    Keys.onPressed: (event) => {
                        if (event.modifiers & Qt.ControlModifier) {
                            if (event.key === Qt.Key_R) {
                                App.library.fetchUnwatchedEpisodes(App.library.listType)
                            }
                        } else {
                            if (event.key === Qt.Key_Tab) {
                                event.accepted = true
                                listTypeComboBox.popup.close()
                                App.library.cycleDisplayingListType() ;
                                listTypeComboBox.currentIndex = App.library.listType
                            }
                        }


                    }

    RowLayout {
        id: topBar
        height: parent.height * 0.07
        anchors {
            topMargin: 5
            leftMargin: 5
            left: parent.left
            right:parent.right
            top: parent.top
        }
        spacing: 5
        CustomComboBox {
            id:listTypeComboBox
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 0.15
            fontSize: 25 * root.fontSizeMultiplier
            text: "text"
            currentIndex: App.library.listType
            onActivated: (index) => {App.library.listType = index}
            model: ListModel{
                ListElement { text: "Watching" }
                ListElement { text: "Planned" }
                ListElement { text: "Paused" }
                ListElement { text: "Dropped" }
                ListElement { text: "Completed" }
            }
        }
        CustomComboBox {
            id: showTypeComboBox
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 0.15
            fontSize: 25 * root.fontSizeMultiplier
            text: "text"
            currentIndex: App.library.model.typeFilter

            onActivated: (index) => App.library.model.typeFilter = index

            model: ListModel{
                ListElement { text: "All"}
                ListElement { text: "Animes"}
                ListElement { text: "Movies"}
                ListElement { text: "Tv Series"}
                ListElement { text: "Variety Shows"}
                ListElement { text: "Documentaries"}
            }
            // NONE = 0,
            // ANIME = 1,
            // MOVIE = 2,
            // TVSERIES = 3,
            // VARIETY = 4,
            // DOCUMENTARY = 5,
        }


        CustomTextField {
            id: titleFilterTextField
            checkedColor: "#727CF5"
            color: "white"
            placeholderText: qsTr("Search")
            placeholderTextColor: "gray"
            text: App.library.model.titleFilter
            Binding {
                target: App.library.model
                property: "titleFilter"
                value: titleFilterTextField.text
            }
            fontSize: 25 * root.fontSizeMultiplier
            Layout.fillHeight: true
            Layout.preferredWidth: 0.3
            Layout.fillWidth: true
        }
        CheckBox {
            onClicked: App.library.model.hasUnwatchedEpisodesOnly = checked
            Component.onCompleted: checked = App.library.model.hasUnwatchedEpisodesOnly
            text: qsTr("Unwatched Only")
            Layout.fillHeight: true
            Layout.preferredWidth: 0.2
            Layout.fillWidth: true
            font.pixelSize: 25 * root.fontSizeMultiplier
            id: hasUnwatchedEpisodesOnlyCheckBox
            indicator: Rectangle {
                implicitWidth: 26
                implicitHeight: 26
                x: hasUnwatchedEpisodesOnlyCheckBox.leftPadding
                y: parent.height / 2 - height / 2
                radius: 3
                border.color: hasUnwatchedEpisodesOnlyCheckBox.checked ? hasUnwatchedEpisodesOnlyCheckBox.down ? "#17a81a" : "#21be2b" : "red"
                id:indicator
                Text {
                    width: 14
                    height: 14
                    x: 1
                    y: -2
                    text: "✔"
                    font.pointSize: 18
                    color: hasUnwatchedEpisodesOnlyCheckBox.down ? "#17a81a" : "#21be2b"
                    visible: hasUnwatchedEpisodesOnlyCheckBox.checked
                }
            }
            contentItem: Text {
                text: hasUnwatchedEpisodesOnlyCheckBox.text
                font: titleFilterTextField.font

                color: "white"
                opacity: hasUnwatchedEpisodesOnlyCheckBox.enabled ? 1.0 : 0.3
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                anchors.left: indicator.right
                anchors.leftMargin: 5

            }

        }

        Text {
            text:`${gridView.count} Show(s)`
            font.pixelSize: 25 * root.fontSizeMultiplier
            color: "white"
            verticalAlignment: Qt.AlignVCenter
            // Layout.fillWidth: true
            Layout.fillHeight: true
            // Layout.preferredWidth: 0.2
            Layout.alignment: Qt.AlignRight
        }

    }



    LibraryGridView {
        id:gridView
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

        onDragFinished: () => {
                            let lastContentY = gridView.contentY
                            App.library.move(gridView.initialDragIndex, gridView.currentDragIndex); // resets contentY to 0
                            gridView.contentY = lastContentY
                            gridView.currentDragIndex = -1
                        }
        onContextMenuRequested: (index) =>{
                                    contextMenu.index = index
                                    contextMenu.popup()
                                }

        anchors {
            left: parent.left
            top: topBar.bottom
            bottom: parent.bottom
            right: parent.right
            rightMargin: 20
        }
        Component.onCompleted: {
            contentY = root.watchListViewLastScrollY
            forceActiveFocus()

        }
    }


    Menu {
        id: contextMenu
        modal: true
        property int index

        Menu {
            title: "Change list type"
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 0
                text: "Watching"
                onTriggered: App.library.changeListTypeAt(contextMenu.index, 0, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 1
                text: "Planned"
                onTriggered: App.library.changeListTypeAt(contextMenu.index, 1, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 2
                text: "Paused"
                onTriggered: App.library.changeListTypeAt(contextMenu.index, 2, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 3
                text: "Dropped"
                onTriggered: App.library.changeListTypeAt(contextMenu.index, 3, -1)
                height: visible ? implicitHeight : 0
            }
            MenuItem {
                visible: listTypeComboBox.currentIndex !== 4
                text: "Completed"
                onTriggered: App.library.changeListTypeAt(contextMenu.index, 4, -1)
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
