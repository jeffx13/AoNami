import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import Kyokou.App.Main
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
    onOpened: {
        if ((loader.currentIndex === 1 && App.play.videoList.count <= 1) ||
                (loader.currentIndex === 2 && App.play.audioList.count <= 1) ||
                (loader.currentIndex === 3 && App.play.subtitleList.count < 1)) {
            loader.setCurrentIndex(0)
        }
    }

    Rectangle {
        id: headers
        anchors{
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: parent.height * 0.15
        color: "black"
        border.color: "white"
        RowLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 5
            Button {
                id: serversButton
                text: qsTr("Servers")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                width: parent.width / 4
                height: parent.height
                onClicked: loader.setCurrentIndex(0)
                contentItem: Text {
                    text: serversButton.text
                    font: serversButton.font
                    opacity: enabled ? 1.0 : 0.3
                    color: loader.currentIndex === 0 ? "#17a81a" : (serversButton.down ? "#17a81a" : "#fcfcfc")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
            Button {
                id: videosButton
                text: qsTr("Videos")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                width: parent.width / 4
                height: parent.height
                onClicked: loader.setCurrentIndex(1)
                enabled: App.play.videoList.count > 1
                contentItem: Text {
                    text: videosButton.text
                    font: videosButton.font
                    opacity: enabled ? 1.0 : 0.3
                    color: loader.currentIndex === 1 ? "#17a81a" : (videosButton.down ? "#17a81a" : "#fcfcfc")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
            Button {
                id: audiosButton
                text: qsTr("Audios")
                enabled: App.play.audioList.count > 1
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                width: parent.width / 4
                height: parent.height
                onClicked: loader.setCurrentIndex(2)
                contentItem: Text {
                    text: audiosButton.text
                    font: audiosButton.font
                    opacity: enabled ? 1.0 : 0.3
                    color: loader.currentIndex === 2 ? "#17a81a" : (audiosButton.down ? "#17a81a" : "#fcfcfc")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
            Button {
                id: subtitlesButton
                text: qsTr("Subtitles")
                enabled: App.play.subtitleList.count > 0
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                width: parent.width / 4
                height: parent.height
                onClicked: loader.setCurrentIndex(3)
                contentItem: Text {
                    text: subtitlesButton.text
                    font: subtitlesButton.font
                    opacity: enabled ? 1.0 : 0.3
                    color: loader.currentIndex === 3 ? "#17a81a" : (subtitlesButton.down ? "#17a81a" : "#fcfcfc")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

            }
        }
    }

    Component {
        id:serversPage
        ListView{
            clip: true
            model: App.play.serverList
            boundsBehavior: Flickable.StopAtBounds
            delegate: ServerVideoAudioListDelegate {
                required property string name
                required property int index
                text: name
                isCurrentIndex: index === App.play.serverList.currentIndex
                width: loader.width
                height: loader.height / 5
                onClicked: {
                    App.play.loadServer(index)
                }
            }
        }
    }
    Component {
        id:videosPage
        ListView{
            clip: true
            id: videoListView
            model: App.play.videoList
            boundsBehavior: Flickable.StopAtBounds
            delegate: ServerVideoAudioListDelegate{
                required property string label
                required property int index
                text: label
                isCurrentIndex: index === App.play.videoList.currentIndex
                width: loader.width
                height: loader.height / 5
                onClicked: {
                    App.play.loadVideo(index)
                }
            }
        }
    }
    Component {
        id:audiosPage
        ListView{
            clip: true
            id: audioListView
            model: App.play.audioList
            boundsBehavior: Flickable.StopAtBounds
            delegate: ServerVideoAudioListDelegate{
                required property string label
                required property int index
                text: label
                isCurrentIndex: index === App.play.audioList.currentIndex
                width: loader.width
                height: loader.height / 5
                onClicked: {
                    App.play.loadAudio(index)
                }
            }


        }
    }

    Component {
        id:subtitlesPage
        ListView{
            clip: true
            id: subtitlesListView
            model: App.play.subtitleList
            boundsBehavior: Flickable.StopAtBounds
            delegate: ServerVideoAudioListDelegate{
                required property string label
                required property int index
                text: label
                isCurrentIndex: index === App.play.subtitleList.currentIndex
                width: loader.width
                height: loader.height / 5
                onClicked: App.play.subtitleList.currentIndex = index
            }
        }
    }



    Loader {
        id: loader

        anchors {
            top: headers.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: 5
            leftMargin: 5
            rightMargin: 5
            topMargin: 3
        }
        property int currentIndex: 0
        function setCurrentIndex(i) {
            currentIndex = i
            switch (i) {
                case 0:
                    sourceComponent = serversPage
                    break;
                case 1:
                    sourceComponent = videosPage
                    break;
                case 2:
                    sourceComponent = audiosPage
                    break;
                case 3:
                    sourceComponent = subtitlesPage
                    break;
            }
        }
        sourceComponent: serversPage

    }


}
