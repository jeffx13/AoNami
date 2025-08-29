import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import Kyokou
import ".."

Item {
    id: controlBar
    
    // Properties
    readonly property bool hovered: hoverHandler.hovered || sliderHovered
    readonly property bool          sliderHovered: timeSlider.hovered || timeSlider.pressed
    property alias         volumeButton: volumeButton
    
    // Required properties
    required property bool isPlaying
    required property int time
    required property int duration
    required property int volume
    
    // Signals
    signal sidebarButtonClicked()
    signal folderButtonClicked()
    signal seekRequested(int time)
    signal playPauseButtonClicked()
    signal settingsButtonClicked()
    signal volumeButtonClicked()
    signal serversButtonClicked()
    signal captionButtonClicked()
    signal stopButtonClicked()

    // Update slider when time changes externally
    onTimeChanged: {
        if (!timeSlider.pressed) {
            timeSlider.value = time
        }
    }

    // Helper function to format time
    function toHHMMSS(seconds) {
        var hours = Math.floor(seconds / 3600)
        seconds -= hours * 3600
        var minutes = Math.floor(seconds / 60)
        seconds -= minutes * 60

        hours = hours < 10 ? "0" + hours : hours
        minutes = minutes < 10 ? "0" + minutes : minutes
        seconds = seconds < 10 ? "0" + seconds : seconds

        hours = hours === "00" ? "" : hours + ':'
        return hours + minutes + ':' + seconds
    }

    // Spacer for when slider is not hovered
    Item {
        id: spacer
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: parent.height * 0.1
    }

    // Main background rectangle
    Rectangle {
        id: backgroundRect
        color: '#d0303030'
        anchors {
            top: controlBar.sliderHovered ? controlBar.top : spacer.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        HoverHandler {
            id: hoverHandler
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        }
        
        // Time slider
        Slider {
            id: timeSlider
            from: 0
            to: controlBar.duration
            focusPolicy: Qt.NoFocus
            hoverEnabled: true
            live: true
            z: backgroundRect.z + 1
            enabled: !mpv.isLoading
            
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                bottom: controlButtons.top
                leftMargin: 10
                rightMargin: 10
            }
            
            onPressedChanged: {
                if (!pressed) { // released
                    controlBar.seekRequested(value)
                }
            }
            
            // Mouse area for cursor shape
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                cursorShape: Qt.PointingHandCursor
            }

            // Slider background
            background: Rectangle {
                x: timeSlider.leftPadding
                implicitHeight: controlBar.sliderHovered ? controlBar.height * 0.22 : controlBar.height * 0.12
                width: timeSlider.availableWidth
                height: implicitHeight
                radius: height / 2
                color: "#2F3B56"

                // Progress indicator
                Rectangle {
                    width: timeSlider.visualPosition * parent.width
                    height: parent.height
                    radius: height / 2
                    color: "#4E5BF2"
                }
            }

            // Slider handle
            handle: Rectangle {
                id: handle
                visible: controlBar.sliderHovered
                width: controlBar.height * 0.2
                height: controlBar.height * 0.2
                radius: width / 2
                color: timeSlider.pressed ? "#f0f0f0" : "#f6f6f6"
                border.color: "#bdbebf"
                x: timeSlider.leftPadding + timeSlider.visualPosition * (timeSlider.availableWidth - width)
            }
        }

        // Control buttons row
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
            
            // Left side controls
            ImageButton {
                id: playPauseButton
                image: controlBar.isPlaying ? "qrc:/Kyokou/resources/images/pause.png" : "qrc:/Kyokou/resources/images/play.png"
                hoverImage: controlBar.isPlaying ? "qrc:/Kyokou/resources/images/pause_hover.png" : "qrc:/Kyokou/resources/images/play_hover.png"
                Layout.preferredWidth: height
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignLeft
                onClicked: controlBar.playPauseButtonClicked()
            }
            
            ImageButton {
                id: stopButton
                image: "qrc:/Kyokou/resources/images/stop.png"
                hoverImage: "qrc:/Kyokou/resources/images/stop_hover.png"
                Layout.preferredWidth: height
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignLeft
                onClicked: controlBar.stopButtonClicked()
            }

            ImageButton {
                id: volumeButton
                image: {
                    if (controlBar.volume === 0) return "qrc:/Kyokou/resources/images/mute_volume.png"
                    if (controlBar.volume < 50) return "qrc:/Kyokou/resources/images/low_volume.png"
                    if (controlBar.volume < 125) return "qrc:/Kyokou/resources/images/mid_volume.png"
                    return "qrc:/Kyokou/resources/images/high_volume.png"
                }
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

            // Left spacer
            Item {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
            }

            // Loading indicator
            Text {
                text: "Loading..."
                color: "white"
                visible: App.play.isLoading || mpv.isLoading
                font.pixelSize: height * 0.8
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter
            }

            BusyIndicator {
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignHCenter
                running: App.play.isLoading || mpv.isLoading
            }

            // Right spacer
            Item {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
            }

            // Right side controls
            ImageButton {
                id: serversButton
                image: "qrc:/Kyokou/resources/images/servers.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: controlBar.serversButtonClicked()
            }

            ImageButton {
                id: captionButton
                image: "qrc:/Kyokou/resources/images/cc.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: controlBar.captionButtonClicked()
            }

            ImageButton {
                id: pipButton
                image: "qrc:/Kyokou/resources/images/pip.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: Globals.togglePip()
            }

            ImageButton {
                id: explorerButton
                image: "qrc:/Kyokou/resources/images/folder.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: controlBar.folderButtonClicked()
            }

            ImageButton {
                id: settingsButton
                image: "qrc:/Kyokou/resources/images/player_settings.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: controlBar.settingsButtonClicked()
            }

            ImageButton {
                id: sidebarButton
                image: "qrc:/Kyokou/resources/images/playlist.png"
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.alignment: Qt.AlignRight
                onClicked: controlBar.sidebarButtonClicked()
            }
        }
    }
}
