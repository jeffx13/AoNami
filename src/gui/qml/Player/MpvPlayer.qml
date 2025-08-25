import QtQuick
import QtQuick.Controls
import Kyokou.App.Main
import "../Components"

MpvObject {
    id: mpv
    property bool autoHideBars: true
    volume: volumeSlider.value
    onPlayNext: App.play.loadNextItem(1)
    Component.onCompleted: root.mpv = mpv

    function copyVideoLink() {
        App.copyToClipboard(mpv.getCurrentVideoUrl().toString())
        mpv.showText("Copied " + mpv.getCurrentVideoUrl().toString())
    }

    function peak(time) {
        controlBar.visible = true
        inactivityTimer.interval = time ? time : 1000
        if (autoHideBars) inactivityTimer.restart()
    }

    function togglePlayPause() {
        if (mpv.state === MpvObject.VIDEO_PLAYING) mpv.pause()
        else mpv.play()
    }

    Connections {
        target: mpv
        function onIsLoadingChanged() {
            if (!mpv.isLoading) root.gotoPage(3)
        }
    }

    MouseArea {
        id: mouseArea
        property point pressPos: Qt.point(0, 0)
        property bool pipDragging: false
        property int dragThreshold: 1
        anchors {
            top: mpv.top
            bottom: controlBar.visible ? controlBar.top : mpv.bottom
            left: mpv.left
            right: mpv.right
        }
        
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: root.pipMode ? Qt.ArrowCursor : controlBar.visible ? Qt.ArrowCursor : Qt.BlankCursor

        onPressed: (mouse) => {
                       if (mouse.button === Qt.LeftButton && root.pipMode) {
                           pressPos = Qt.point(mouse.x, mouse.y)
                           pipDragging = false
                       }
                   }
        onDoubleClicked: {
            if (root.pipMode) root.togglePip()
            else root.toggleFullscreen()
        }
        onClicked: (mouse) => {
                       if (mouse.button === Qt.RightButton)
                           contextMenu.popup()
                       
                   }
        onPositionChanged: (mouse) => {
            mpv.peak()
            if (root.pipMode && mouseArea.pressed && !pipDragging) {
                var dx = mouse.x - pressPos.x
                var dy = mouse.y - pressPos.y
                if (Math.abs(dx) > dragThreshold || Math.abs(dy) > dragThreshold) {
                    pipDragging = true
                    root.startSystemMove()
                }
            }
        }
        onCanceled: pipDragging = false
        onReleased: (mouse) => { 
            if (root.pipMode && pipDragging) {
                root.ensureFullyVisibleOnScreen(); pipDragging = false 
            } else if (!pipDragging) {
                mpv.togglePlayPause()
            }
            
            }
        

        Timer {
            id: inactivityTimer
            interval: 2000
            onTriggered: {
                if (!mpv.visible) return
                if (!mouseArea.pressed && !controlBar.hovered) {
                    controlBar.visible = false
                }
            }
        }
    }

    ControlBar {
        id: controlBar
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        
        z: mpv.z + 1
        visible: false
        height: parent.height * 0.06

        isPlaying: mpv.state === MpvObject.VIDEO_PLAYING || mpv.state === MpvObject.TV_PLAYING
        time: mpv.time
        duration: mpv.duration
        volume: mpv.volume

        onPlayPauseButtonClicked: mpv.togglePlayPause()
        onSeekRequested: (time) => { mpv.seek(time) }
        onSidebarButtonClicked: playlistBar.toggle()
        onFolderButtonClicked: folderDialog.open()
        onServersButtonClicked: serverListPopup.toggle()
        onVolumeButtonClicked: { mpv.muted = !mpv.muted }
        onSettingsButtonClicked: settingsPopup.toggle(2)
        onCaptionButtonClicked: settingsPopup.toggle(1)
        onStopButtonClicked: mpv.stop()
        
        Popup {
            id: volumePopup
            width: 40
            height: 120
            
            Slider {
                id: volumeSlider
                anchors.fill: parent
                from: 0
                to: 100
                value: 50
                stepSize: 1
                snapMode: Slider.SnapAlways
                orientation: Qt.Vertical
            }
        }
    }

    SettingsPopup {
        id: settingsPopup
        x: parent.width - width - 10
        y: parent.height - height - controlBar.height - 10
        width: parent.width / 3
        height: parent.height / 3
        
        onClosed: timer.restart()
        
        function toggle(index) {
            timer.stop()
            if (settingsPopup.isOpen && settingsPopup.currentIndex === index) {
                settingsPopup.close()
                settingsPopup.isOpen = false
                mpvPage.forceActiveFocus()
            } else {
                settingsPopup.open()
                settingsPopup.isOpen = true
                settingsPopup.currentIndex = index
            }
        }
        
        Timer {
            id: timer
            interval: 500
            repeat: false
            running: false
            onTriggered: settingsPopup.isOpen = false
        }
    }

    AppMenu {
        id: contextMenu
        modal: true

        AppMenu {
            title: "Open"
            modal: false
            Action {
                text: "Open File <font color='#A0A0A0'>(E)</font>"
                onTriggered: fileDialog.open()
            }
            Action {
                text: "Open Folder <font color='#A0A0A0'>(Ctrl+E)</font>"
                onTriggered: folderDialog.open()
            }
        }

        Action {
            text: "Paste link <font color='#A0A0A0'>(Ctrl+P)</font>"
            onTriggered: App.play.openUrl("", true)
        }

        Action {
            text: "Copy link <font color='#A0A0A0'>(Ctrl+C)</font>"
            onTriggered: mpv.copyVideoLink()
        }

        Action {
            text: "Reload <font color='#A0A0A0'>(Ctrl+R)</font>"
            onTriggered: App.play.reload()
        }
    }
}
