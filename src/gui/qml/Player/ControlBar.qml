import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import Kyokou.App.Main

Item {
    id: controlBar
    readonly property bool hovered: hoverHandler.hovered || sliderHovered
    signal sidebarButtonClicked()
    signal folderButtonClicked()
    signal seekRequested(int time)
    signal playPauseButtonClicked()
    signal settingsButtonClicked()
    signal volumeButtonClicked()
    signal serversButtonClicked()
    signal captionButtonClicked()
    signal stopButtonClicked()


    required property bool isPlaying
    required property int time
    required property int duration
    required property int volume
    property alias volumeButton: volumeButton

    onTimeChanged:{
        if (!timeSlider.pressed)
            timeSlider.value = time;
    }


    function toHHMMSS(seconds){
        var hours = Math.floor(seconds / 3600);
        seconds -= hours*3600;
        var minutes = Math.floor(seconds / 60);
        seconds -= minutes*60;

        hours = hours < 10 ? "0" + hours : hours;
        minutes = minutes < 10 ? "0" + minutes : minutes;
        seconds = seconds < 10 ? "0" + seconds : seconds;

        hours = hours === "00" ? "" : hours + ':';
        return hours + minutes + ':' + seconds;
    }

    property bool sliderHovered: timeSlider.hovered || timeSlider.pressed

    Item{
        id:spacer
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: parent.height * 0.1
    }


    Rectangle {
        color: '#d0303030'
        id:backgroundRect
        anchors {
            top: controlBar.sliderHovered ? controlBar.top : spacer.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        HoverHandler {
            id: hoverHandler
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            // cursorShape: Qt.PointingHandCursor
        }
        Slider {
            id: timeSlider
            from: 0
            to: controlBar.duration
            focusPolicy: Qt.NoFocus
            hoverEnabled: true
            live: true
            z: backgroundRect.z + 1
            enabled: !mpvPlayer.isLoading
            anchors {
                top:parent.top
                left: parent.left
                right: parent.right
                bottom: controlButtons.top
                leftMargin: 10
                rightMargin: 10
            }
            onPressedChanged: {
                if (!pressed)  // released
                    controlBar.seekRequested(value);
            }
            MouseArea{
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                cursorShape: Qt.PointingHandCursor
            }

            background: Rectangle {
                x: timeSlider.leftPadding
                implicitHeight: controlBar.sliderHovered ? controlBar.height * 0.22 : controlBar.height * 0.12
                width: timeSlider.availableWidth
                height: implicitHeight
                radius: height / 2
                color: "#2F3B56"

                Rectangle {
                    width: timeSlider.visualPosition * parent.width
                    height: parent.height
                    radius: height / 2
                    color: "#4E5BF2"
                }
            }

            handle: Rectangle {
                id: handle
                visible: controlBar.sliderHovered
                width: controlBar.height * 0.2
                height: controlBar.height * 0.2
                radius: width / 2
                color: timeSlider.pressed ? "#f0f0f0" : "#f6f6f6"
                border.color: "#bdbebf"

                x: timeSlider.leftPadding + timeSlider.visualPosition * (timeSlider.availableWidth - width)
                // y: timeSlider.availableHeight / 2 - height
            }
        }


        RowLayout {
            id: controlButtons
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                leftMargin: 10
                rightMargin: 10
            }
            height: controlBar.height * 0.8
            ImageButton {
                id: playPauseButton
                image: controlBar.isPlaying ? "qrc:/resources/images/pause.png" : "qrc:/resources/images/play.png"
                hoverImage: controlBar.isPlaying ? "qrc:/resources/images/pause_hover.png" : "qrc:/resources/images/play_hover.png"
                Layout.preferredWidth: height
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignLeft
                onClicked: controlBar.playPauseButtonClicked()
            }
            ImageButton {
                id: stopButton
                image: "qrc:/resources/images/stop.png"
                hoverImage: "qrc:/resources/images/stop_hover.png"
                Layout.preferredWidth: height
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignLeft
                onClicked: controlBar.stopButtonClicked()
            }

            ImageButton {
                id: volumeButton
                image: controlBar.volume === 0 ? "qrc:/resources/images/mute_volume.png" :
                                           controlBar.volume < 25 ? "qrc:/resources/images/low_volume.png" :
                                                             controlBar.volume < 75 ? "qrc:/resources/images/mid_volume.png" : "qrc:/resources/images/high_volume.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignLeft
                onClicked: controlBar.volumeButtonClicked()
            }

            Text {
                id: timeText
                text: `${controlBar.toHHMMSS(controlBar.time)} / ${controlBar.toHHMMSS(controlBar.duration)}`
                color: "white"
                font.pixelSize: height * 0.8
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignLeft
            }

            Item{
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
            }

            Text{
                text: "Loading..."
                color: "white"
                visible: (App.play.isLoading || mpvPlayer.isLoading)
                font.pixelSize: height * 0.8
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

            }

            BusyIndicator  {
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignHCenter
                running: (App.play.isLoading || mpvPlayer.isLoading)
            }

            Item{
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
            }

            ImageButton {
                id: serversButton
                image: "qrc:/resources/images/servers.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignLeft
                onClicked: controlBar.serversButtonClicked()
            }

            ImageButton {
                id: captionButton
                image: "qrc:/resources/images/cc.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: controlBar.captionButtonClicked()
            }

            ImageButton {
                id: pipButton
                image: "qrc:/resources/images/pip.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: root.pipMode = !root.pipMode
            }

            ImageButton {
                id: explorerButton
                image: "qrc:/resources/images/folder.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: controlBar.folderButtonClicked()
            }

            ImageButton {
                id: settingsButton
                image: "qrc:/resources/images/player_settings.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: controlBar.settingsButtonClicked()
            }

            ImageButton {
                id: sidebarButton
                image: "qrc:/resources/images/playlist.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: controlBar.sidebarButtonClicked()
            }

        }
    }

}







