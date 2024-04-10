import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ListView {
    id:episodeListView
    // ScrollBar.vertical: ScrollBar {
    //     active: true
    // }
    clip: true
    model:app.currentShow.episodeList

    Component.onCompleted: {
        if (app.currentShow.episodeList.lastWatchedIndex !== -1)
            episodeListView.positionViewAtIndex(app.currentShow.episodeList.lastWatchedIndex, ListView.Center)
    }

    boundsMovement: Flickable.StopAtBounds

    delegate: Rectangle {
        id: delegateRect
        width: episodeListView.width
        height: 60
        border.width: 2
        border.color: "black"
        color: app.currentShow.episodeList.lastWatchedIndex === index ? "red" : "black"
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: delegateRect.border.color = "white"
            onExited: delegateRect.border.color = delegateRect.color
            onClicked:{
                app.playFromEpisodeList(index)
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
                id:episodeStr
                text:  model.fullTitle
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
                    app.downloadCurrentShow(index)
                }
            }

        }



    }

}

