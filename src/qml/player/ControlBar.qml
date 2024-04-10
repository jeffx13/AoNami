import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import "../components"
import MpvPlayer 1.0
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
            top: sliderHovered ? controlBar.top : spacer.bottom
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
            to: duration
            focusPolicy: Qt.NoFocus
            hoverEnabled: true
            live: true
            z:backgroundRect.z + 1
            enabled: !mpv.isLoading
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
                    seekRequested(value);
            }
            MouseArea{
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                cursorShape: Qt.PointingHandCursor
            }

            background: Rectangle {
                x: timeSlider.leftPadding
                implicitHeight: sliderHovered ? controlBar.height * 0.2 : controlBar.height * 0.1
                width: timeSlider.availableWidth
                height: implicitHeight
                color: "#828281" //grey

                Rectangle {
                    width: timeSlider.visualPosition * parent.width
                    height: parent.height
                    color: "#00AEEC" //turqoise
                }
            }

            handle: Rectangle {
                id: handle
                visible: sliderHovered
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
                source: isPlaying ? "qrc:/resources/images/pause.png" : "qrc:/resources/images/play.png"
                Layout.preferredWidth: height
                Layout.fillHeight: true
                onClicked: playPauseButtonClicked()
            }
            ImageButton {
                id: volumeButton
                source: volume === 0 ? "qrc:/resources/images/mute_volume.png" :
                                           volume < 25 ? "qrc:/resources/images/low_volume.png" :
                                                             volume < 75 ? "qrc:/resources/images/mid_volume.png" : "qrc:/resources/images/high_volume.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                onClicked: volumeButtonClicked()
            }
            ImageButton {
                id: serversButton
                source: "qrc:/resources/images/servers.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                onClicked: serversButtonClicked()
            }

            Text {
                id: timeText
                text: `${toHHMMSS(time)} / ${toHHMMSS(duration)}`
                color: "white"
                font.pixelSize: height * 0.8
                Layout.fillHeight: true
            }
            //spacer
            Item{
                Layout.fillWidth: true
            }

            ImageButton {
                id: pipButton
                source: "qrc:/resources/images/pip.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                onClicked: root.pipMode = true
            }

            ImageButton {
                id: explorerButton
                source: "qrc:/resources/images/folder.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                onClicked: folderButtonClicked()
            }

            ImageButton {
                id: settingsButton
                source: "qrc:/resources/images/player_settings.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                onClicked: settingsButtonClicked()
            }

            ImageButton {
                id: sidebarButton
                source: "qrc:/resources/images/playlist.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                onClicked: sidebarButtonClicked()
            }

        }
    }

}







