import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../Components"
import Kyokou.App.Main

Item {
    id: mpvPage
    focus: true
    
    property alias playListSideBar: playlistBar
    property int volumeStep: 5
    property real speedStepNormal: 0.1
    property real speedStepDouble: 0.2
    property int inactivityTimeout: 2000
    property real normalSpeed: 1.0
    property bool isDoubleSpeed: false

    MpvPlayer {
        id: mpvPlayer
        anchors {
            left: mpvPage.left
            right: playlistBar.visible ? playlistBar.left : mpvPage.right
            top: mpvPage.top
            bottom: mpvPage.bottom
        }

        DropArea {
            id: dropArea
            anchors.fill: parent
            
            onEntered: (drag) => {
                drag.accept(Qt.LinkAction)
            }
            
            onDropped: (drop) => {
                for (var i = 0; i < drop.urls.length; i++) {
                    App.play.openUrl(drop.urls[i], false)
                }
            }
        }

        ServerListPopup {
            id: serverListPopup
            anchors.centerIn: parent
            width: mpvPage.width / 2.5
            height: mpvPage.height / 2.5
            visible: false
            onClosed: mpvPage.forceActiveFocus()
            function toggle() { if (opened) close(); else open() }
        }
    }



    PlayListSideBar {
        id: playlistBar
        anchors {
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
        id: folderDialog
        currentFolder: "file:///" + App.downloader.workDir
        onAccepted: {
            App.play.openUrl(folderDialog.selectedFolder, true)
            mpvPage.forceActiveFocus()
        }
    }
    FileDialog {
        id: fileDialog
        currentFolder: "file:///" + App.downloader.workDir
        fileMode: FileDialog.OpenFile
        nameFilters: [
            "All files (*)",
            "Video and Audio files (*.mp4 *.mkv *.avi *.mp3 *.flac *.wav *.ogg *.webm *.m3u8 *.ts *.mov)",
            "Subtitle files (*.srt *.ass *.ssa *.vtt *.sub *.idx)",
        ]
        
        onAccepted: {
            App.play.openUrl(fileDialog.selectedFile, true)
            mpvPage.forceActiveFocus()
        }
    }
    onVisibleChanged: if (visible) playlistBar.scrollToIndex(App.play.currentModelIndex)

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
        switch (event.key) {
        case Qt.Key_Shift:
            isDoubleSpeed = false
        }
    }
    Keys.onPressed: event => {if (event.modifiers & Qt.ControlModifier) handleCtrlModifiedKeyPress(event); else handleKeyPress(event)}
    function handleKeyPress(event) {
        if (event.modifiers & Qt.AltModifier) return
        switch (event.key) {
        case Qt.Key_V: serverListPopup.toggle(); break
        case Qt.Key_M: mpvPlayer.muted = !mpvPlayer.muted; break
        case Qt.Key_Z:
        case Qt.Key_Left: mpvPlayer.seek(mpvPlayer.time - 5); break
        case Qt.Key_X:
        case Qt.Key_Right: mpvPlayer.seek(mpvPlayer.time + 5); break
        case Qt.Key_Tab:
        case Qt.Key_Asterisk: App.play.showCurrentItemName(); break
        case Qt.Key_Slash: mpvPlayer.peak(); break
        case Qt.Key_E: fileDialog.open(); break
        case Qt.Key_Shift: isDoubleSpeed = true; break
        case Qt.Key_P: playlistBar.toggle(); break
        case Qt.Key_W: playlistBar.visible = !playlistBar.visible; break
        case Qt.Key_Up:
        case Qt.Key_Q: mpvPlayer.volume += volumeStep; break
        case Qt.Key_Down:
        case Qt.Key_A: mpvPlayer.volume -= volumeStep; break
        case Qt.Key_Space:
        case Qt.Key_Clear: mpvPlayer.togglePlayPause(); break
        case Qt.Key_PageUp: App.play.loadNextItem(1); break
        case Qt.Key_Home: App.play.loadNextItem(-1); break
        case Qt.Key_PageDown: mpvPlayer.seek(mpvPlayer.time + 90); break
        case Qt.Key_End: mpvPlayer.seek(mpvPlayer.time - 90); break
        case Qt.Key_Plus:
        case Qt.Key_D: incrementSpeed(); break;
        case Qt.Key_Minus:
        case Qt.Key_S: decrementSpeed(); break;
        case Qt.Key_Escape:
            if (root.pipMode) root.togglePip()
            else if (root.fullscreen) root.toggleFullscreen()
            break
        case Qt.Key_C:
            mpvPlayer.subVisible = !mpvPlayer.subVisible
            mpvPlayer.showText(mpvPlayer.subVisible ? "Subtitles enabled" : "Subtitles disabled")
            break

        case Qt.Key_R:
            if (mpvPlayer.speed > 1.0) mpvPlayer.setSpeed(1.0)
            else mpvPlayer.setSpeed(2.0)
            break
        case Qt.Key_F:
            if (root.pipMode) root.togglePip()
            else root.toggleFullscreen()
            break
        default: mpvPlayer.sendKeyPress(event.text); break
        }
    }

    function handleCtrlModifiedKeyPress(event) {
        switch (event.key) {
        case Qt.Key_Z: mpvPlayer.seek(mpvPlayer.time - 90); break
        case Qt.Key_X: mpvPlayer.seek(mpvPlayer.time + 90); break
        case Qt.Key_V: App.play.openUrl("", true); break
        case Qt.Key_R: App.play.reload(); break
        case Qt.Key_A: { playlistBar.visible = false; root.togglePip(); break }
        case Qt.Key_C: mpvPlayer.copyVideoLink(); break
        case Qt.Key_Control: break
        case Qt.Key_S:
            if (event.modifiers & Qt.ShiftModifier) App.play.loadNextPlaylist(-1)
            else App.play.loadNextItem(-1)
            break
        case Qt.Key_D:
            if (event.modifiers & Qt.ShiftModifier) App.play.loadNextPlaylist(1)
            else App.play.loadNextItem(1)
            break
        case Qt.Key_E:
            if (event.modifiers & Qt.ShiftModifier) Qt.openUrlExternally("file:///" + App.downloader.workDir)
            else folderDialog.open()
            break
        default:
            mpvPlayer.sendKeyPress("CTRL+" + event.text)
            break
        }
    }
}
