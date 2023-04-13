import QtQuick 2.15
import QtQuick.Controls 2.15
import MpvPlayer 1.0
Item{
    id:mpvPage
    property var mpv:mpvObject
    property bool fullscreen: false

    function setFullscreen(shouldFullscreen){
        if(!fullscreen){
            setMaximised(shouldFullscreen)
            mpvPage.fullscreen = shouldFullscreen
        }else{
            setMaximised(shouldFullscreen)
            mpvPage.fullscreen=shouldFullscreen
        }
    }

    anchors{
        bottom:parent.bottom
        right: parent.right
        left: parent.left
    }
    Rectangle{
        id:playlistBar
        visible: false
        anchors{
            right: parent.right
            top: parent.top
            bottom: controlBar.top
        }
        z:2
        width: 200
        color: "white"
        Text {
            id:playlistNameText
            text: app.playlistModel.showName
            font.pixelSize: 16
            font.bold: true
            wrapMode:Text.Wrap
            anchors{
                top: parent.top
                right: parent.right
                left: parent.left
            }
            height: contentHeight
        }
        ListView {
            id: listView
            model: app.playlistModel
            clip:true
            anchors{
                top: playlistNameText.bottom
                right: parent.right
                left: parent.left
                bottom: parent.bottom
            }
            currentIndex: app.playlistModel.currentIndex

            onCurrentIndexChanged: {
                positionViewAtIndex(currentIndex, ListView.PositionAtCenter)
            }
            delegate: Rectangle {
                width: listView.width
                height: itemText.height + 10
                radius: 4
                clip: true
                color: index === app.playlistModel.currentIndex ? 'red': index % 2 === 0 ? "gray" : "white"
                Text  {
                    id:itemText
                    text: model.numberTitle
                    font.pixelSize: 14
                    wrapMode:Text.Wrap
                    anchors{
                        left: parent.left
                        right: parent.right
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    //                        onClicked: listView.currentIndex = index
                    onDoubleClicked: {
                        console.log(model.numberTitle)
                        app.playlistModel.loadSource(index)
                    }
                }
            }
        }
        function toggle(){
            playlistBar.visible = !playlistBar.visible
            if(playlistBar.visible){
                if (mpv.state == MpvObject.VIDEO_PLAYING){
                    mpv.pause()
                }
            }else{
                if (mpv.state == MpvObject.VIDEO_PAUSED){
                    mpv.play()
                }
            }
        }
    }


    MpvObject{
        id:mpvObject
        z:0
        volume: volumeSlider.value
        anchors.fill: parent

        MouseArea{
            function peak(){
                controlBar.visible=true
                if (autoHideBars) {
                    timer.restart();
                }
            }

            id:mouseArea

            anchors.fill: mpvObject
            property bool autoHideBars: true
            hoverEnabled: true
            onMouseXChanged: {
                mouseArea.cursorShape = Qt.ArrowCursor;
                peak()
            }
            Timer{
                id:doubleClickTimer
                interval: 300
            }
            onClicked: (mouse)=>{
                           if(doubleClickTimer.running)
                           {
                               setFullscreen(!fullscreen)
                               if(mpv.state == MpvObject.VIDEO_PLAYING) {mpv.pause() }else {mpv.play()}
                               doubleClickTimer.stop()
                           }
                           else{
                               if(mpv.state == MpvObject.VIDEO_PLAYING) {mpv.pause() }else {mpv.play()}
                               doubleClickTimer.restart()
                           }


                       }
            Timer {
                id: timer
                interval: 1000
                onTriggered: {
                    if (mouseArea.pressed === true) {
                        return;
                    }
                    if (!controlBar.contains(controlBar.mapFromItem(mouseArea, mouseArea.mouseX, mouseArea.mouseY)))
                    {
                        mouseArea.cursorShape = Qt.BlankCursor;
                        controlBar.visible = false;
                    }
                }
            }
        }
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

    ControlBar{
        id:controlBar
        visible: false
        z:1000
        anchors{
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        height: 30
        isPlaying: mpv.state === MpvObject.VIDEO_PLAYING || mpv.state === MpvObject.TV_PLAYING
        time: mpv.time
        duration: mpv.duration
        onPlayPauseButtonClicked: mpv.state === MpvObject.VIDEO_PLAYING ? mpv.pause() : mpv.play()
        onStopButtonClicked: mpv.stop()
        onSeekRequested: (time)=>mpv.seek(time);
        onVolumeButtonClicked: {
            volumePopup.x = mpv.mapFromItem(volumeButton, 0, 0).x;
            volumePopup.y = mpv.mapFromItem(volumeButton, 0, 0).y - volumePopup.height;
            volumePopup.visible = true;
        }
        onSidebarButtonClicked: playlistBar.toggle()
    }

    Keys.enabled: true
    focus: mpvPage.visible
    Keys.onPressed: event => handleKeyPress(event)
    function handleKeyPress(event){
        if(event.modifiers & Qt.ControlModifier){
            if (event.key === Qt.Key_1) {
                mpv.loadAnime4K(1)
            } else if (event.key === Qt.Key_2) {
                mpv.loadAnime4K(2)
            } else if (event.key === Qt.Key_3) {
                mpv.loadAnime4K(3)
            } else if (event.key === Qt.Key_4) {
                mpv.loadAnime4K(4)
            } else if (event.key === Qt.Key_5) {
                mpv.loadAnime4K(5)
            } else if (event.key === Qt.Key_6) {
                mpv.loadAnime4K(6)
            } else if (event.key === Qt.Key_0) {
                mpv.loadAnime4K(0)
            } else if (event.key === Qt.Key_Z) {
                mpv.seek(mpv.time - 90)
            } else if (event.key === Qt.Key_X) {
                mpv.seek(mpv.time + 90)
            } else if (event.key === Qt.Key_S) {
                mpv.playPrecedingItem()
            } else if (event.key === Qt.Key_D) {
                mpv.playNextItem()
            }
        } else if (event.key === Qt.Key_Escape && fullscreen) {
            setFullscreen(false)
        } else if (event.key === Qt.Key_W) {
            playlistBar.visible = !playlistBar.visible
        } else if (event.key === Qt.Key_Up) {
            volumeSlider.value += 5
        } else if (event.key === Qt.Key_Down) {
            volumeSlider.value -= 5
        } else if (event.key === Qt.Key_Q) {
            volumeSlider.value += 5
        } else if (event.key === Qt.Key_A) {
            volumeSlider.value -= 5
        } else if (event.key === Qt.Key_Space) {
            if(mpv.state === MpvObject.VIDEO_PLAYING) mpv.pause()
            else mpv.play()
        }else if (event.key === Qt.Key_Clear) {
            if(mpv.state === MpvObject.VIDEO_PLAYING) mpv.pause()
            else mpv.play()
        }else if (event.key === Qt.Key_PageUp) {
            mpv.playNextItem()
        }else if (event.key === Qt.Key_Home) {
            mpv.playPrecedingItem()
        }
        else if (event.key === Qt.Key_PageDown) {
            mpv.seek(mpv.time + 90)
        }
        else if (event.key === Qt.Key_End) {
            mpv.seek(mpv.time - 90)
        }
        else if (event.key === Qt.Key_Plus) {
            mpv.setSpeed(mpv.speed + 0.1)
        }
        else if (event.key === Qt.Key_Minus) {
            mpv.setSpeed(mpv.speed - 0.1)
        }
        else if (event.key === Qt.Key_S) {
            mpv.setSpeed(mpv.speed - 0.1)
        } else if (event.key === Qt.Key_D) {
            mpv.setSpeed(mpv.speed + 0.1)
        } else if (event.key === Qt.Key_R) {
            if(mpv.speed > 1.0) mpv.setSpeed(1.0)
            else mpv.setSpeed(2.0)
        } else if (event.key === Qt.Key_F) {
            setFullscreen(!fullscreen)
        } else if (event.key === Qt.Key_M) {
            mpv.mute()
        } else if (event.key === Qt.Key_Z || event.key === Qt.Key_Left) {
            mpv.seek(mpv.time - 5)
        } else if (event.key === Qt.Key_X || event.key === Qt.Key_Right) {
            mpv.seek(mpv.time + 5)
        } else if (event.key === Qt.Key_Tab) {
            mpv.showText(app.playlistModel.currentItemName)
        }else if (event.key === Qt.Key_Asterisk) {
            mpv.showText(app.playlistModel.currentItemName)
        }else if (event.key === Qt.Key_Slash) {
            mouseArea.peak()
        }else if (event.key === Qt.P) {
            playlistBar.toggle()
        }
    }

    KeyNavigation.tab:mpvPage


}
