pragma ComponentBehavior: Bound
import QtQuick 2.15
// import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import Kyokou 1.0
ListView {
    id:episodeListView

    clip: true
    model: App.currentShow.episodeList
    property real lastWatchedIndex: App.currentShow.episodeList.reversed ? episodeListView.count - 1 - App.currentShow.episodeList.lastWatchedIndex : App.currentShow.episodeList.lastWatchedIndex
    function correctIndex(index) {
        return App.currentShow.episodeList.reversed ? episodeListView.count - 1 - index : index
    }
    Component.onCompleted: {
        if (App.currentShow.episodeList.lastWatchedIndex !== -1)
            episodeListView.positionViewAtIndex(lastWatchedIndex, ListView.Center)
    }

    boundsMovement: Flickable.StopAtBounds

    delegate: Rectangle {
        id: delegateRect
        width: episodeListView.width
        height: 60
        border.width: 2
        border.color: "black"
        color: episodeListView.lastWatchedIndex === index ? "red" : "black"
        required property string fullTitle
        required property int index
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: delegateRect.border.color = "white"
            onExited: delegateRect.border.color = delegateRect.color
            onClicked:{
                App.playFromEpisodeList(correctIndex(index))
            }
        }
        RowLayout {
            anchors{
                left:parent.left
                right:parent.right
                top:parent.top
                bottom: parent.bottom
                margins: 3
            }

            Text {
                id: episodeStr
                text:  fullTitle
                font.pixelSize: 20 * root.fontSizeMultiplier
                Layout.fillHeight: true
                Layout.fillWidth: true
                elide: Text.ElideRight
                wrapMode: Text.Wrap
                color: "white"
            }

            ImageButton {
                source: "qrc:/resources/images/download-button.png"
                Layout.preferredWidth: height
                Layout.preferredHeight: 50
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                onClicked: {
                    enabled = false;
                    source = "qrc:/resources/images/download_selected.png"
                    App.downloadCurrentShow(correctIndex(index), 1)
                }
            }

        }



    }

}

