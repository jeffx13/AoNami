import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
Popup {
    id:serverListPopup
    visible: true
    background: Rectangle{
        radius: 10
        color: "black"
    }
    opacity: 0.7
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    modal: true

    Text {
        id: serversText
        text: qsTr("Servers")
        font.pixelSize: parent.height * 0.14
        horizontalAlignment: Text.AlignHCenter
        anchors{
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: parent.height * 0.15
        color: "white"
    }
    ListView{
        id:serversListView
        clip: true
        model: app.playlist.serverList
        boundsBehavior: Flickable.StopAtBounds
        anchors {
            top:serversText.bottom
            left: parent.left
            right: parent.right
            bottom:parent.bottom
            bottomMargin: 5
            leftMargin: 5
            rightMargin: 5
            topMargin: 3
        }

        delegate: Rectangle {
            required property string name
            required property string link
            required property int index
            width: serversListView.width
            height: 60 * root.fontSizeMultiplier
            color: app.playlist.serverList.currentIndex === index ? "purple" : "black"
            border.width: 2
            border.color: "white"
            ColumnLayout {
                anchors {
                    fill: parent
                    margins: 3
                }
                Text {
                    id:serverNameText
                    text: name
                    font.pixelSize: 25 * root.fontSizeMultiplier
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    elide: Text.ElideRight
                    wrapMode: Text.Wrap
                    color: "white"
                }
                Text {
                    id: serverLinkText
                    text: link
                    font.pixelSize: 25 * root.fontSizeMultiplier
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    elide: Text.ElideRight
                    wrapMode: Text.Wrap
                    color: "white"
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: app.playlist.serverList.currentIndex = index
            }

        }
    }
}
