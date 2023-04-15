import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import "../components"
Control {
    id: controlBar

    // Color settings

    background: Rectangle {
        implicitHeight: 40
        color: '#d0303030'//SkinColor.controlbar
    }

    signal playPauseButtonClicked()
    signal stopButtonClicked()
    signal settingsButtonClicked()
    signal volumeButtonClicked()
    signal sidebarButtonClicked()
    signal folderButtonClicked()
    signal seekRequested(int time)

    property bool isPlaying: false
    property int time: 0
    property int duration: 0
    property alias volumeButton: volumeButton

    function toHHMMSS(seconds) {
        var hours = Math.floor(seconds / 3600);
        seconds -= hours*3600;
        var minutes = Math.floor(seconds / 60);
        seconds -= minutes*60;

        if (hours   < 10) {hours   = "0"+hours;}
        if (minutes < 10) {minutes = "0"+minutes;}
        if (seconds < 10) {seconds = "0"+seconds;}
        return hours+':'+minutes+':'+seconds;
    }

    RowLayout {
        spacing: 10
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10

        ImageButton {
            id: playPauseButton
            image: isPlaying ?
                       (true ? "qrc:/resources/images/pause_lightgrey.png" : "qrc:/resources/images/pause_grey.png") :
                       (true ? "qrc:/resources/images/play_lightgrey.png" : "qrc:/resources/images/play_grey.png")
            hoverImage: isPlaying ?
                            (true ? "qrc:/resources/images/pause_lightgrey_on.png" : "qrc:/resources/images/pause_grey_on.png") :
                            (true ? "qrc:/resources/images/play_lightgrey_on.png" : "qrc:/resources/images/play_grey_on.png")
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            onClicked: playPauseButtonClicked()
        }

        ImageButton {
            id: stopButton
            image: true ? "qrc:/resources/images/stop_lightgrey.png" : "qrc:/resources/images/stop_grey.png"
            hoverImage: true ? "qrc:/resources/images/stop_lightgrey_on.png" : "qrc:/resources/images/stop_grey_on.png"
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            onClicked: stopButtonClicked()
        }

        ImageButton {
            id: volumeButton
            image: true ? "qrc:/resources/images/volume_lightgrey.png" : "qrc:/resources/images/volume_grey.png"
            hoverImage: true ? "qrc:/resources/images/volume_lightgrey_on.png" : "qrc:/resources/images/volume_grey_on.png"
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            onClicked: volumeButtonClicked()
        }

        Label {
            id: timeText
            text: toHHMMSS(time)
            color: "white"
        }

        Slider {
            id: timeSlider
            from: 0
            to: duration
            focusPolicy: Qt.NoFocus

            Layout.fillWidth: true
            Layout.preferredHeight: 24
            onPressedChanged: {
                if (!pressed)  // released
                    seekRequested(value);
            }
            background: Rectangle {
                x: timeSlider.leftPadding
                y: timeSlider.topPadding + timeSlider.availableHeight / 2 - height / 2
                implicitWidth: 200
                implicitHeight: 4
                width: timeSlider.availableWidth
                height: implicitHeight
                radius: 2
                color: "#bdbebf"

                Rectangle {
                    width: timeSlider.visualPosition * parent.width
                    height: parent.height
                    color: "#21be2b"
                    radius: 2
                }
            }

            handle: Rectangle {
                x: timeSlider.leftPadding + timeSlider.visualPosition * (timeSlider.availableWidth - width)
                y: timeSlider.topPadding + timeSlider.availableHeight / 2 - height / 2
                implicitWidth: 26
                implicitHeight: 26
                radius: 13
                color: timeSlider.pressed ? "#f0f0f0" : "#f6f6f6"
                border.color: "#bdbebf"
            }
        }

        Label {
            id: durationText
            text: toHHMMSS(duration)
            color: "white"
        }

        ImageButton {
            id: explorerButton
            image: "qrc:/resources/images/folder_lightgrey.png"
            hoverImage: "qrc:/resources/images/folder_lightgrey_on.png"
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            onClicked: folderButtonClicked()
        }

        ImageButton {
            id: settingsButton
            image: true ? "qrc:/resources/images/settings_lightgrey.png" : "qrc:/resources/images/settings_grey.png"
            hoverImage: true ? "qrc:/resources/images/settings_lightgrey_on.png" : "qrc:/resources/images/settings_grey_on.png"
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            onClicked: settingsButtonClicked()
        }

        ImageButton {
            id: sidebarButton
            image: true ? "qrc:/resources/images/playlist_lightgrey.png" : "qrc:/resources/images/playlist_grey.png"
            hoverImage: true ? "qrc:/resources/images/playlist_lightgrey_on.png" : "qrc:/resources/images/playlist_grey_on.png"
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            onClicked: sidebarButtonClicked()
        }
    }

    onTimeChanged: {
        if (!timeSlider.pressed)
            timeSlider.value = time;
    }
}
