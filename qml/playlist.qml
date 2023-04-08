
import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3

Item {
    id: playlist

    signal openFileRequested()
    signal openUrlRequested()
    z:100
    visible: true
    GridLayout {
        anchors.fill: parent
        columns: 3

        Label {
            text: qsTr("Playlist")
            font.pixelSize: 16
            font.bold: true
            Layout.columnSpan: 3
        }

        ScrollView {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: listView

                model: app.playlistModel
                delegate: Rectangle {
                    id: delegateRect
                    width: listView.width
                    height: 40
                    color: "#f2f2f2"
                    border.color: "#ccc"
                    border.width: 1
                    radius: 5
                    Text {
                        text: "Episode " + model.title
                        font.pixelSize: 16
                        anchors.centerIn: parent
                        color: "#444"
                        font.family: "Arial"
                    }

                    MouseArea {
                        anchors.fill: delegateRect
                        onEntered: delegateRect.color = "#ccc"
                        onExited: delegateRect.color = "#f2f2f2"
                        onClicked: (mouse)=>{
                                       console.log("loading episode "+model.number)
//                                       listView.loadEpisode(index)
                                   }
                    }

                }
            }
        }

        Button {
            id: addButton
            text: qsTr("Add")
            implicitWidth: 55
            onClicked: {
                addMenu.x = addButton.x
                addMenu.y = addButton.y - addMenu.height
                addMenu.open()
            }
        }

        Button {
            id: delButton
            text: qsTr("Del")
            implicitWidth: 55
            onClicked: PlaylistModel.removeItem(listView.currentIndex)
        }

        Button {
            id: clearButton
            text: qsTr("Clear")
            implicitWidth: 55
            onClicked: PlaylistModel.clear()
        }
    }

    Menu {
        id: addMenu
        width: 100
        MenuItem {
            text: qsTr("File...")
            onTriggered: openFileRequested()
        }
        MenuItem {
            text: qsTr("Url...")
            onTriggered: openUrlRequested()
        }
    }
}
