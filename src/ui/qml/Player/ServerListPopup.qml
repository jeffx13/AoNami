import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AoNami
import ".."
Popup {
    id:serverListPopup
    visible: true
    background: Rectangle{
        radius: 12
        color: "black"
    }
    opacity: 0.8
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 0.8; duration: 120; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale"; from: 0.96; to: 1.0; duration: 120; easing.type: Easing.OutBack }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0.8; to: 0; duration: 120; easing.type: Easing.InCubic }
            NumberAnimation { property: "scale"; from: 1.0; to: 0.98; duration: 120; easing.type: Easing.InCubic }
        }
    }

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    modal: true
    onOpened: {
        const listCounts = [App.play.serverList.count, mpv.videoList.count, mpv.audioList.count, mpv.subtitleList.count];
        if (listCounts[loader.currentIndex] <= 1) {
            loader.setCurrentIndex(listCounts.indexOf(Math.max(...listCounts)));
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
            focusPolicy: Qt.NoFocus
            Button {
                id: serversButton
                text: qsTr("Servers") + ` (${App.play.serverList.count})`
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                font.pixelSize: Globals.sp(16)
                width: parent.width / 4
                height: parent.height
                onClicked: loader.setCurrentIndex(0)
                focusPolicy: Qt.NoFocus
                contentItem: Text {
                    text: serversButton.text
                    font: serversButton.font
                    opacity: enabled ? 1.0 : 0.3
                    color: loader.currentIndex === 0 ? "#4E5BF2" : (serversButton.down ? "#4E5BF2" : "#fcfcfc")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
            Button {
                id: videosButton
                text: qsTr("Videos") + ` (${mpv.videoList.count})`
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                font.pixelSize: Globals.sp(16)
                width: parent.width / 4
                height: parent.height
                onClicked: loader.setCurrentIndex(1)
                focusPolicy: Qt.NoFocus
                enabled: mpv.videoList.count > 0
                contentItem: Text {
                    text: videosButton.text
                    font: videosButton.font
                    opacity: enabled ? 1.0 : 0.3
                    color: loader.currentIndex === 1 ? "#4E5BF2" : (videosButton.down ? "#4E5BF2" : "#fcfcfc")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
            Button {
                id: audiosButton
                text: qsTr("Audios") + ` (${mpv.audioList.count})`
                enabled: mpv.audioList.count > 0
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                font.pixelSize: Globals.sp(16)
                width: parent.width / 4
                height: parent.height
                focusPolicy: Qt.NoFocus
                onClicked: loader.setCurrentIndex(2)
                contentItem: Text {
                    text: audiosButton.text
                    font: audiosButton.font
                    opacity: enabled ? 1.0 : 0.3
                    color: loader.currentIndex === 2 ? "#4E5BF2" : (audiosButton.down ? "#4E5BF2" : "#fcfcfc")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
            Button {
                id: subtitlesButton
                text: qsTr("Subtitles") + ` (${mpv.subtitleList.count})`
                enabled: mpv.subtitleList.count > 0
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                font.pixelSize: Globals.sp(16)
                width: parent.width / 4
                height: parent.height
                focusPolicy: Qt.NoFocus
                onClicked: loader.setCurrentIndex(3)
                contentItem: Text {
                    text: subtitlesButton.text
                    font: subtitlesButton.font
                    opacity: enabled ? 1.0 : 0.3
                    color: loader.currentIndex === 3 ? "#4E5BF2" : (subtitlesButton.down ? "#4E5BF2" : "#fcfcfc")
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
            id: serversListView
            clip: true
            model: App.play.serverList
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: App.play.serverList.currentIndex
            delegate: TrackDelegate {
                required property string name
                required property int index
                text: name
                isCurrentIndex: index === currentIndex
                width: loader.width
                height: loader.height / 5
                onClicked: App.play.loadServer(index)
            }
            onVisibleChanged: if (visible) positionViewAtIndex(currentIndex, ListView.Beginning)
            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Beginning)
        }
    }
    Component {
        id:videosPage
        ListView {
            clip: true
            id: videoListView
            model: mpv.videoList
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: mpv.videoList.currentIndex
            delegate: TrackDelegate {
                required property string title
                required property int index
                text: title
                isCurrentIndex: index === currentIndex
                width: loader.width
                height: loader.height / 5
                onClicked: mpv.setVideoIndex(index)
            }
            onVisibleChanged: if (visible) positionViewAtIndex(currentIndex, ListView.Beginning)
            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Beginning)
        }
    }
    Component {
        id:audiosPage
        ListView{
            clip: true
            id: audioListView
            model: mpv.audioList
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: mpv.audioList.currentIndex
            delegate: TrackDelegate {
                required property string title
                required property int index
                text: title
                isCurrentIndex: index === currentIndex
                width: loader.width
                height: loader.height / 5
                onClicked: {
                    mpv.setAudioIndex(index)
                }
            }
            onVisibleChanged: if (visible) positionViewAtIndex(currentIndex, ListView.Beginning)
            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Beginning)

        }
    }

    Component {
        id:subtitlesPage
        ListView{
            clip: true
            id: subtitlesListView
            model: mpv.subtitleList
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: mpv.subtitleList.currentIndex
            delegate: TrackDelegate{
                required property string title
                required property int index
                text: title
                isCurrentIndex: index === currentIndex
                width: loader.width
                height: loader.height / 5
                onClicked: mpv.setSubIndex(index)
            }
            onVisibleChanged: if (visible) positionViewAtIndex(currentIndex, ListView.Beginning)
            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Beginning)
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
