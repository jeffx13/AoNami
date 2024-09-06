import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"
import QtQuick.Layouts 1.15
import Kyokou 1.0
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
                        if (event.key === Qt.Key_Tab) {
                            event.accepted = true
                            listTypeComboBox.popup.close()
                            App.library.cycleDisplayingListType() ;
                            listTypeComboBox.currentIndex = App.library.listType
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
            Layout.preferredWidth: 0.2 * libraryPage.width
            fontSize: 20
            text: "text"
            currentIndex: App.library.listType
            onActivated: (index) => {App.library.listType = index}
            model: ListModel{
                ListElement { text: "Watching" }
                ListElement { text: "Planned" }
                ListElement { text: "On Hold" }
                ListElement { text: "Dropped" }
                ListElement { text: "Completed" }
            }
        }
        CustomComboBox {
            id: showTypeComboBox
            Layout.fillHeight: true
            Layout.preferredWidth: 0.2 * libraryPage.width
            fontSize: 20
            text: "text"
            currentIndex: App.library.model.typeFilter

            onActivated: App.library.model.typeFilter = index

            model: ListModel{
                ListElement { text: "All"}
                ListElement { text: "Animes"}
                ListElement { text: "Movies"}
                ListElement { text: "Tv Series"}
                ListElement { text: "Variety Shows"}
                ListElement { text: "Documentaries"}
            }

            // MOVIE = 1,
            // TVSERIES,
            // VARIETY,
            // ANIME,
            // DOCUMENTARY,
            // NONE
        }


        CustomTextField {
            id: titleFilterTextField
            checkedColor: "#727CF5"
            color: "white"
            placeholderText: qsTr("Search")
            placeholderTextColor: "gray"
            text:App.library.model.titleFilter
            Binding {
                target: App.library.model
                property: "titleFilter"
                value: titleFilterTextField.text
            }
            fontSize: 20
            Layout.fillHeight: true
            Layout.preferredWidth: 0.4 * libraryPage.width

        }

        Text {
            text:`${gridView.count} show(s)`
            font.pixelSize: 20 * root.fontSizeMultiplier
            color: "white"
            verticalAlignment: Qt.AlignVCenter
            Layout.fillHeight: true
            Layout.preferredWidth: contentWidth
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
        onMoveRequested: {
            App.library.move(gridView.dragFromIndex, gridView.dragToIndex);
            gridView.contentY=gridView.lastY
        }
        onContextMenuRequested: (index) =>{
                                    contextMenu.index = index
                                    contextMenu.popup()
                                }

        anchors{
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
        MenuItem {
            text: "Remove from library"
            onTriggered:  {
                App.library.removeAt(contextMenu.index)
            }
        }

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
                text: "On Hold"
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
    }

}
