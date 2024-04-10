import QtQuick 2.15
import QtQuick.Controls
import MpvPlayer 1.0
import QtQuick.Dialogs
import QtQuick.Layouts 1.15

MpvObject {
    id: mpv
    volume: volumeSlider.value
    property var lastPos

    property bool autoHideBars: true
    onPlayNext: app.playlist.playNextItem()
    Component.onCompleted: {
        root.mpv = mpv
    }

    function peak(time){
        controlBar.visible = true
        inactivityTimer.interval = time ? time : 1000
        if (autoHideBars) {
            inactivityTimer.restart();
        }
    }
    // Toggle play/pause based on the current video state
    function togglePlayPause() {
        if (mpv.state === MpvObject.VIDEO_PLAYING) {
            mpv.pause();
        } else {
            mpv.play();
        }
    }

    MouseArea {
        id: mouseArea
        property var clickPos
        property point lastPos
        property bool moved: false
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: root.pipMode ? Qt.ArrowCursor : controlBar.visible ? Qt.ArrowCursor : Qt.BlankCursor
        anchors {
            top: mpv.top
            bottom: controlBar.visible ? controlBar.top : mpv.bottom
            left: mpv.left
            right: mpv.right
        }

        onPressed: (mouse) => {
                       if (mouse.button === Qt.LeftButton && root.pipMode) {
                           // If in PiP mode, store the initial click position for dragging
                           clickPos = Qt.point(mouse.x, mouse.y);
                       }
                   }

        onReleased: (mouse) => {
                        if (mouse.button === Qt.LeftButton){
                            if (root.pipMode) {
                                mouseArea.clickPos = null; // Reset click position after drag
                                if (!mouseArea.moved) mpv.togglePlayPause(); // Toggle play/pause only if no drag action occurred
                                mouseArea.moved = false; // Reset moved flag
                            } else {
                                mpv.togglePlayPause(); // Toggle play/pause in non-PiP mode
                            }

                            // Handle double-click for exiting PiP mode or toggling fullscreen
                            if (doubleClickTimer.running) {
                                if (root.pipMode) {
                                    root.pipMode = false; // Exit PiP mode
                                } else {
                                    root.fullscreen = !root.fullscreen; // Toggle fullscreen
                                }
                            } else {
                                doubleClickTimer.restart(); // Restart double-click timer
                            }
                        } else {
                            contextMenu.popup()
                        }
                    }

        onPositionChanged: (mouse) => {
                               if (root.pipMode && clickPos) {
                                   // Handle drag in PiP mode
                                   let newX = mouse.x - clickPos.x + root.x;
                                   let newY = mouse.y - clickPos.y + root.y;
                                   mouseArea.moved = true
                                   root.x = Math.max(0, Math.min(Screen.desktopAvailableWidth - root.width, newX));
                                   root.y = Math.max(0, Math.min(Screen.desktopAvailableHeight - root.height, newY));
                               } else {
                                   // Handle cursor showing and auto-hide logic
                                   mpv.peak();
                                   mouseArea.lastPos = Qt.point(mouse.x, mouse.y);
                                   inactivityTimer.restart();
                               }
                           }

        Timer {
            id: doubleClickTimer
            interval: 300 // Double-click threshold
        }

        Timer {
            id: inactivityTimer
            interval: 2000
            onTriggered: {
                if (!mpvPage.visible) return
                var newPos = Qt.point(mouseArea.mouseX, mouseArea.mouseY)
                if (newPos === mouseArea.lastPos &&
                        !mouseArea.pressed && !settingsPopup.visible && !serverListPopup.visible
                        && !controlBar.hovered
                        ) {
                    // If the mouse hasn't moved, hide the cursor
                    controlBar.visible = false;
                }
                else {
                    // If the mouse has moved, update the last position and restart the timer
                    mouseArea.lastPos = newPos;
                    inactivityTimer.restart();
                }
            }
        }
    }

    ControlBar {
        id:controlBar
        z: mpv.z + 1

        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        visible: false
        height: parent.height * 0.06

        isPlaying: mpv.state === MpvObject.VIDEO_PLAYING || mpv.state === MpvObject.TV_PLAYING
        time: mpv.time
        duration: mpv.duration
        volume: mpv.volume

        onPlayPauseButtonClicked: mpv.togglePlayPause()
        onSeekRequested: (time)=>{mpv.seek(time)};
        onSidebarButtonClicked: playlistBar.toggle()
        onFolderButtonClicked: {root.fullscreen = false; folderDialog.open()}
        onSettingsButtonClicked: settingsPopup.toggle()
        onServersButtonClicked: serverListPopup.toggle()
        onVolumeButtonClicked: {
            volumePopup.x = mpv.mapFromItem(volumeButton, 0, 0).x;
            volumePopup.y = mpv.mapFromItem(volumeButton, 0, 0).y - volumePopup.height;
            volumePopup.visible = true;
        }

        Popup {
            id: volumePopup
            width: 40
            height: 120
            Slider {
                id: volumeSlider
                from: 0
                to: 100
                value: 50
                stepSize: 1
                snapMode: Slider.SnapAlways
                anchors.fill: parent
                orientation: Qt.Vertical
            }
        }

    }

    SettingsPopup {
        id:settingsPopup
        x:parent.width-width - 10
        y:parent.height - height - controlBar.height - 10
        width: parent.width / 3
        height: parent.height / 3.5
        visible: false
        onClosed: mpvPage.forceActiveFocus()
        function toggle() {
            if(settingsPopup.opened) {
                settingsPopup.close()
            } else {
                settingsPopup.open()
            }
        }
    }

    ServerListPopup {
        id:serverListPopup
        anchors.centerIn: parent
        width: parent.width / 2.7
        height: parent.height / 2.5
        visible: false
        onClosed: mpvPage.forceActiveFocus()
        function toggle() {
            if(serverListPopup.opened) {
                serverListPopup.close()
            } else {
                serverListPopup.open()
            }
        }
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: "Copy link"
            onTriggered:  {
                mpv.copyVideoLink()
            }
        }
        MenuItem {
            text: "Paste link"
            onTriggered:  {
                app.playlist.pasteOpen()
            }
        }
        MenuItem {
            text: "Reload"
            onTriggered:  {
                mpv.reload()
            }
        }
        MenuItem {
            text: "Open Folder"
            onTriggered:  {
                folderDialog.open()
            }
        }
        MenuItem {
            text: "Open File"
            onTriggered:  {
                fileDialog.open()
            }
        }
    }

    FolderDialog {
        id:folderDialog
        currentFolder: "file:///D:/TV/"
        onAccepted: {
            app.playlist.openUrl(folderDialog.selectedFolder, true)
            mpvPage.forceActiveFocus()
        }
    }
    FileDialog {
        id:fileDialog
        currentFolder: "file:///D:/TV/"
        onAccepted: {
            app.playlist.openUrl(fileDialog.selectedFile, true)
            mpvPage.forceActiveFocus()
        }

        fileMode: FileDialog.OpenFile
        nameFilters: ["Video files (*.mp4 *.mkv *.avi *.mp3 *.flac *.wav *.ogg *.webm *.m3u8)"]

    }


}
