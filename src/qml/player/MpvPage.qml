import QtQuick 2.15
import QtQuick.Controls

import QtQuick.Dialogs
import QtQuick.Layouts 1.15
import "../components"
import Kyokou.App.Main
import "."
Item{
    id:mpvPage
    focus: true
    property alias playListSideBar: playlistBar

    MpvPlayer {
        id:mpvPlayer
        anchors {
            left: mpvPage.left
            right: playlistBar.visible ? playlistBar.left : mpvPage.right
            top: mpvPage.top
            bottom: mpvPage.bottom
        }
    }

    DropArea {
            id: dropArea;
            anchors.fill: parent
            onEntered: (drag) => {
                drag.accept(Qt.LinkAction);
            }
            onDropped: (drop) => {
                for (var i = 0; i < drop.urls.length; i++) {
                    App.play.openUrl(drop.urls[i], false)
                }
            }
        }


    PlayListSideBar {
        id:playlistBar
        anchors{
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: root.width * 0.2
        visible: false
        function toggle() {
            if (root.pipMode) return
            playlistBar.visible = !playlistBar.visible
            mpvPage.forceActiveFocus()
        }
    }

    FolderDialog {
        id:folderDialog
        currentFolder: "file:///C:/"

        onAccepted: {
            App.saveTimeStamp()
            App.play.openUrl(folderDialog.selectedFolder, true)
            mpvPage.forceActiveFocus()
        }
    }

    FileDialog {
        id:fileDialog
        currentFolder: "file:///C:/"
        onAccepted: {
            App.saveTimeStamp()
            App.play.openUrl(fileDialog.selectedFile, true)
            mpvPage.forceActiveFocus()
        }

        fileMode: FileDialog.OpenFile
        nameFilters: ["Video files (*.mp4 *.mkv *.avi *.mp3 *.flac *.wav *.ogg *.webm *.m3u8 *.ts *.mov)"]

    }

    onVisibleChanged: if (visible) playlistBar.scrollToIndex(App.play.currentModelIndex)

    ServerListPopup {
        id:serverListPopup
        anchors.centerIn: parent
        width: parent.width / 2.5
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
    property real normalSpeed: 1.0
    property bool isDoubleSpeed: false
    onIsDoubleSpeedChanged: {
        if (isDoubleSpeed) {
            normalSpeed = mpvPlayer.speed
            mpvPlayer.setSpeed(mpvPlayer.speed * 2)
        } else {
            mpvPlayer.setSpeed(normalSpeed)
        }
    }

    Keys.enabled: true

    Keys.onReleased: event => {
                         switch(event.key) {
                             case Qt.Key_Shift:
                             isDoubleSpeed = false
                             break
                         }
                     }

    Keys.onPressed: event => handleKeyPress(event)

    function playOffset(playlistOffset, itemOffset) {
        App.saveTimeStamp()
        App.play.loadOffset(playlistOffset, itemOffset)
    }



    function handleKeyPress(event){
        if (event.modifiers & Qt.ControlModifier){
            if (event.key === Qt.Key_W) return
            handleCtrlModifiedKeyPress(event)
            return
        }

        if (event.modifiers & Qt.AltModifier){
            return
        }

        switch (event.key){
        case Qt.Key_Escape:
            if (resizeAnime.running) return
            if (root.pipMode) {
                root.pipMode = false
                return
            }
            root.fullscreen = false
            break;
        case Qt.Key_P:
            playlistBar.toggle();
            break;
        case Qt.Key_W:
            playlistBar.visible = !playlistBar.visible;
            break;
        case Qt.Key_Up:
            mpvPlayer.volume += 5;
            break;
        case Qt.Key_Down:
            mpvPlayer.volume -= 5;
            break;
        case Qt.Key_Q:
            mpvPlayer.volume += 5;
            break;
        case Qt.Key_A:
            mpvPlayer.volume -= 5;
            break;
        case Qt.Key_C:
            mpvPlayer.subVisible = !mpvPlayer.subVisible
            mpvPlayer.showText(mpvPlayer.subVisible ? "Subtitles enabled" : "Subtitles disabled")
            break;
        case Qt.Key_Space:
        case Qt.Key_Clear:
            mpvPlayer.togglePlayPause()
            break;
        case Qt.Key_PageUp:
            playOffset(0, 1)
            break;
        case Qt.Key_Home:
            playOffset(0, -1)
            break;
        case Qt.Key_PageDown:
            mpvPlayer.seek(mpvPlayer.time + 90);
            break;
        case Qt.Key_End:
            mpvPlayer.seek(mpvPlayer.time - 90);
            break;
        case Qt.Key_Plus:
        case Qt.Key_D:
            if (isDoubleSpeed) {
                normalSpeed += 0.1
                mpvPlayer.setSpeed(mpvPlayer.speed + 0.2)
            } else {
                mpvPlayer.setSpeed(mpvPlayer.speed + 0.1)
            }

            break;
        case Qt.Key_Minus:
        case Qt.Key_S:
            if (isDoubleSpeed) {
                normalSpeed -= 0.1
                mpvPlayer.setSpeed(mpvPlayer.speed - 0.2)
            } else {
                mpvPlayer.setSpeed(mpvPlayer.speed - 0.1)
            }
            break;
        case Qt.Key_R:
            if (mpvPlayer.speed > 1.0)
                mpvPlayer.setSpeed(1.0)
            else
                mpvPlayer.setSpeed(2.0)
            break;
        case Qt.Key_F:
            if (resizeAnime.running) return
            if (root.pipMode) {
                root.pipMode = false
            } else {
                fullscreen = !fullscreen
            }
            break;
        case Qt.Key_V:
            serverListPopup.toggle()
            break;
        case Qt.Key_M:
            mpvPlayer.mute();
            break;
        case Qt.Key_Z:
        case Qt.Key_Left:
            mpvPlayer.seek(mpvPlayer.time - 5);
            break;
        case Qt.Key_X:
        case Qt.Key_Right:
            mpvPlayer.seek(mpvPlayer.time + 5);
            break;
        case Qt.Key_Tab:
        case Qt.Key_Asterisk:
            App.play.showCurrentItemName()
            break;
        case Qt.Key_Slash:
            mpvPlayer.peak()
            break;
        case Qt.Key_E:
            folderDialog.open()
            break;
        case Qt.Key_C:

            break;
        case Qt.Key_Shift:
            isDoubleSpeed = true
            break;
        default:
            var keyLetter = event.text
            mpvPlayer.sendKeyPress(event.text)
        }

    }
    function handleCtrlModifiedKeyPress(event){
        switch(event.key) {
        case Qt.Key_Z:
            mpvPlayer.seek(mpvPlayer.time - 90)
            break;
        case Qt.Key_X:
            mpvPlayer.seek(mpvPlayer.time + 90)
            break;
        case Qt.Key_S:
            if (event.modifiers & Qt.ShiftModifier) {
                playOffset(-1, 0)
            } else {
                playOffset(0, -1)
            }
            break;
        case Qt.Key_D:
            if (event.modifiers & Qt.ShiftModifier) {
                playOffset(1, 0)
            } else {
                playOffset(0, 1)
            }
            break;
        case Qt.Key_V:
            App.saveTimeStamp()
            App.play.openUrl("", true)
            break;
        case Qt.Key_R:
            App.play.reload()
            break;
        case Qt.Key_A:
            root.togglePipMode()
            break;
        case Qt.Key_E:
            if (event.modifiers & Qt.ShiftModifier)
                Qt.openUrlExternally("file:///" + App.downloader.workDir)
            else
                fileDialog.open()
            break;
        case Qt.Key_C:
            mpvPlayer.copyVideoLink()
            break;
        case Qt.Key_Control:
            break;
        default:
            var keyLetter = event.text
            // console.log("Key pressed: " + keyLetter + " " + event.key)
            mpvPlayer.sendKeyPress("CTRL+" + keyLetter)
        }

    }


}

