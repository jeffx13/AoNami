import QtQuick 2.15
import QtQuick.Controls
import MpvPlayer 1.0
import QtQuick.Dialogs
import QtQuick.Layouts 1.15
import "../components"
Item{
    id:mpvPage
    focus: true
    property alias playListSideBar: playlistBar
    LoadingScreen {
        id:loadingScreen
        z: parent.z + 1
        loading: mpvPage.visible && (app.play.isLoading || mpvPlayer.isLoading)
        cancellable: false
        timeoutEnabled:false
    }

    Connections {
        target: mpvPlayer
        function onIsLoadingChanged() {
            if (!mpvPlayer.isLoading) {
                sideBar.gotoPage(3)
                if (app.play.subtitleList.currentIndex > -1) {
                    mpv.addSubtitle(app.play.subtitleList.currentSubtitle)
                    mpv.subVisible = true
                }
            }
        }
    }
    Connections {
        target: app.play.subtitleList
        function onCurrentIndexChanged() {
            if (app.play.subtitleList.currentIndex > -1) {
                mpv.addSubtitle(app.play.subtitleList.currentSubtitle)
                mpv.subVisible = true
            }
        }
    }

    MpvPlayer {
        id:mpvPlayer
        // focus: true
        anchors {
            left: parent.left
            right: playlistBar.visible ? playlistBar.left : parent.right
            top: parent.top
            bottom: parent.bottom
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
        currentFolder: "file:///D:/TV/"
        onAccepted: {
            app.play.openUrl(folderDialog.selectedFolder, true)
            mpvPage.forceActiveFocus()
        }
    }
    FileDialog {
        id:fileDialog
        currentFolder: "file:///D:/TV/"
        onAccepted: {
            app.play.openUrl(fileDialog.selectedFile, true)
            mpvPage.forceActiveFocus()
        }

        fileMode: FileDialog.OpenFile
        nameFilters: ["Video files (*.mp4 *.mkv *.avi *.mp3 *.flac *.wav *.ogg *.webm *.m3u8)"]

    }

    onVisibleChanged: if (visible) playlistBar.scrollToIndex(app.play.currentIndex)

    Keys.enabled: true
    Keys.onPressed: event => handleKeyPress(event)
    Keys.onReleased: event => {
                         switch(event.key) {
                             case Qt.Key_Shift:
                             mpvPlayer.setSpeed(mpvPlayer.speed / 2)
                         }
                     }

    function handleCtrlModifiedKeyPress(event){
        switch(event.key) {
        case Qt.Key_1:
            mpvPlayer.loadAnime4K(1)
            break;
        case Qt.Key_2:
            mpvPlayer.loadAnime4K(2)
            break;
        case Qt.Key_3:
            mpvPlayer.loadAnime4K(3)
            break;
        case Qt.Key_4:
            mpvPlayer.loadAnime4K(4)
            break;
        case Qt.Key_5:
            mpvPlayer.loadAnime4K(5)
            break;
        case Qt.Key_0:
            mpvPlayer.loadAnime4K(0)
            break;
        case Qt.Key_Z:
            mpvPlayer.seek(mpvPlayer.time - 90)
            break;
        case Qt.Key_X:
            mpvPlayer.seek(mpvPlayer.time + 90)
            break;
        case Qt.Key_S:
            app.play.playPrecedingItem()
            break;
        case Qt.Key_D:
            app.play.playNextItem()
            break;
        case Qt.Key_V:
            app.play.pasteOpen()
            break;
        case Qt.Key_R:
            app.play.reload()
            break;
        case Qt.Key_O:
            if (event.modifiers & Qt.ShiftModifier)
                fileDialog.open()
            else
                folderDialog.open()
            break;
        case Qt.Key_C:
            mpvPlayer.copyVideoLink()
            break;
        }
    }

    function handleKeyPress(event){
        if (event.modifiers & Qt.ControlModifier){
            if (event.key === Qt.Key_W) return
            handleCtrlModifiedKeyPress(event)
        }else{
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
                break;
            case Qt.Key_Space:
            case Qt.Key_Clear:
                mpvPlayer.togglePlayPause()
                break;
            case Qt.Key_PageUp:
                app.play.playNextItem();
                break;
            case Qt.Key_Home:
                app.play.playPrecedingItem();
                break;
            case Qt.Key_PageDown:
                mpvPlayer.seek(mpvPlayer.time + 90);
                break;
            case Qt.Key_End:
                mpvPlayer.seek(mpvPlayer.time - 90);
                break;
            case Qt.Key_Plus:
            case Qt.Key_D:
                mpvPlayer.setSpeed(mpvPlayer.speed + 0.1);
                break;
            case Qt.Key_Minus:
            case Qt.Key_S:
                mpvPlayer.setSpeed(mpvPlayer.speed - 0.1);
                break;
            case Qt.Key_R:
                if (mpvPlayer.speed > 1.0)
                    mpvPlayer.setSpeed(1.0)
                else
                    mpvPlayer.setSpeed(2.0)
                break;
            case Qt.Key_F:
                if (resizeAnime.running) return
                if (root.pipMode)
                {
                    root.pipMode = false
                } else {
                    // playerFillWindow = !playerFillWindow
                    // fullscreen = playerFillWindow
                    fullscreen = !fullscreen
                }

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
                mpvPlayer.showText(app.play.currentItemName);
                break;
            case Qt.Key_Slash:
                mpvPlayer.peak()
                break;
            case Qt.Key_C:
                break;
            case Qt.Key_Shift:
                mpvPlayer.setSpeed(mpvPlayer.speed * 2)
            }
        }
    }


}

