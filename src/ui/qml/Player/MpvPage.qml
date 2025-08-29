import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../Components"
import QtCore
import Kyokou
import ".."
import Qt5Compat.GraphicalEffects

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



    MpvObject {
        id: mpv
        property bool autoHideBars: true
        volume: 100
        onPlayNext: App.play.loadNextItem(1)
        Component.onCompleted: Globals.mpv = mpv
        anchors {
            left: mpvPage.left
            right: playlistBar.visible ? playlistBar.left : mpvPage.right
            top: mpvPage.top
            bottom: mpvPage.bottom
        }

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
                if (!mpv.isLoading) Globals.gotoPage(3)
            }
        }

        DropArea {
            id: dropArea
            anchors.fill: parent

            onEntered: (drag) => drag.accept(Qt.LinkAction)

            onDropped: (drop) => {
                           for (var i = 0; i < drop.urls.length; i++) {
                               App.play.openUrl(drop.urls[i], false)
                           }
                       }
        }

        Popup  {
            id:settingsPopup
            modal: true
            dim: false
            x: parent.width - width - 5
            y: parent.height - height - controlBar.height - 10
            width: parent.width / 2.5
            height: parent.height / 2.5
            function toggle() {
                if (Globals.pipMode) { Globals.togglePip(); return }
                if (opened) close()
                else {
                    open()
                    serverListPopup.close()
                    playlistBar.visible = false
                }
            }
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            background: Item {
                id: bgRoot
                implicitWidth: 420
                implicitHeight: 320
                Rectangle {
                    id: bgCard
                    anchors.fill: parent
                    radius: 12
                    color: "#0F172A"
                    border.color: "#ffffff"
                    border.width: 1
                    clip: true
                }
                DropShadow {
                    anchors.fill: bgCard
                    source: bgCard
                    horizontalOffset: 0
                    verticalOffset: 10
                    radius: 20
                    samples: 28
                    color: "#00000066"
                    transparentBorder: true
                }
            }
            opacity: 1.0
            enter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 140; easing.type: Easing.OutCubic }
                    NumberAnimation { property: "scale"; from: 0.96; to: 1.0; duration: 160; easing.type: Easing.OutBack }
                }
            }
            exit: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120; easing.type: Easing.InCubic }
                    NumberAnimation { property: "scale"; from: 1.0; to: 0.98; duration: 120; easing.type: Easing.InCubic }
                }
            }


            property alias currentIndex: tabBar.currentIndex

            TabBar {
                id: tabBar
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    leftMargin: 2
                    rightMargin: 2
                }
                currentIndex: 0
                height: Math.min(Math.max(44, parent.height * 0.16), 58)
                background: Rectangle { color: "#0B1220" }
                contentItem: ListView {
                    id: view
                    orientation: ListView.Horizontal
                    boundsBehavior: Flickable.StopAtBounds
                    model: ListModel {
                        ListElement { text: qsTr("General") }
                        ListElement { text: qsTr("Skipping") }
                    }
                    delegate: Button {
                        text: model.text
                        width: Math.max(96, view.width / view.count)
                        height: view.height
                        onClicked: tabBar.currentIndex = index
                        focusPolicy: Qt.NoFocus
                        background: Rectangle {
                            radius: 10
                            color: tabBar.currentIndex === index ? "#162036" : "#0F172A"
                            border.color: tabBar.currentIndex === index ? "#4E5BF2" : "#1F2937"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: model.text
                            color: tabBar.currentIndex === index ? "#BFC7FF" : "#E5E7EB"
                            font.pixelSize: Globals.sp(18)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }


            }
            StackLayout {
                id:stackView
                width: parent.width
                anchors {
                    top: tabBar.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                currentIndex: tabBar.currentIndex
                // General
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 5
                    RowLayout {
                        id: rowSubtitles
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        Text { text: qsTr("Subtitles"); color: "#E5E7EB"; Layout.fillWidth: true; Layout.preferredWidth: 1; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 3
                            Layout.fillHeight: true
                            AppSwitch {
                                id: subVisibleSwitch
                                anchors.centerIn: parent
                                focusPolicy: Qt.NoFocus
                                checked: mpv.subVisible
                                onToggled: mpv.subVisible = checked
                            }
                        }
                    }
                    RowLayout {
                        id: rowMute
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        spacing: 10
                        Text { text: qsTr("Mute"); color: "#E5E7EB"; Layout.fillWidth: true; Layout.preferredWidth: 1; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 3
                            Layout.fillHeight: true
                            AppSwitch {
                                id: muteSwitch
                                anchors.centerIn: parent
                                focusPolicy: Qt.NoFocus
                                checked: mpv.muted
                                onToggled: mpv.muted = checked
                            }
                        }
                    }
                    RowLayout {
                        id: rowVolume
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        spacing: 10
                        Text { text: qsTr("Volume"); color: "#E5E7EB"; Layout.fillWidth: true; Layout.preferredWidth: 1; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        AppSlider { from: 0; to: 200; value: mpv.volume; unitSuffix: "%"; decimals: 0; onMoved: mpv.volume = value; Layout.fillWidth: true; Layout.preferredWidth: 3 }
                    }
                    RowLayout {
                        id: rowSpeed
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        spacing: 10
                        Text { text: qsTr("Speed"); color: "#E5E7EB"; Layout.fillWidth: true; Layout.preferredWidth: 1; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        AppSlider { from: 0.1; to: 4.0; stepSize: 0.05; value: mpv.speed; unitSuffix: "x"; decimals: 2; onMoved: mpv.speed = value; Layout.fillWidth: true; Layout.preferredWidth: 3 }
                    }
                }

                // Skipping
                ColumnLayout {
                            id: skippingPage
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 5
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 2
                                Item { Layout.fillWidth: true; Layout.preferredWidth: 0.25 }
                                Text { text: "Start"; color: "#E5E7EB"; Layout.fillWidth: true; Layout.preferredWidth: 0.25; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                Text { text: "Length"; color: "#E5E7EB"; Layout.fillWidth: true; Layout.preferredWidth: 0.25; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                Item { Layout.fillWidth: true; Layout.preferredWidth: 0.25 }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 5
                                Text { text: "Skip OP"; color: "#E5E7EB"; Layout.fillWidth: true; Layout.preferredWidth: 0.25; Layout.fillHeight: true; verticalAlignment: Text.AlignVCenter }
                                AppSpinBox {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 0.25
                                    value: mpv.skipOPStart
                                    from: 0
                                    to: mpv.duration
                                    focusPolicy: Qt.NoFocus
                                    stepSize: 10
                                    onValueModified: mpv.skipOPStart = value;
                                }
                                AppSpinBox {
                                    id: skipOPLength
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 0.25
                                    value: mpv.skipOPLength
                                    from: 0
                                    to: mpv.duration
                                    focusPolicy: Qt.NoFocus
                                    stepSize: 10
                                    onValueModified: mpv.skipOPLength = value;
                                }
                                AppCheckBox {
                                    id: skipOPCheckBox
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 0.25
                                    focusPolicy: Qt.NoFocus
                                    checked: mpv.skipOP
                                    onToggled: mpv.skipOP = !mpv.skipOP;
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 5
                                Text { text: "Skip ED"; color: "#E5E7EB"; Layout.fillWidth: true; Layout.preferredWidth: 0.25; Layout.fillHeight: true; verticalAlignment: Text.AlignVCenter }
                                Item {Layout.fillWidth: true; Layout.preferredWidth: 0.25}
                                AppSpinBox {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 0.25
                                    value: mpv.skipEDLength
                                    from: 0
                                    to: mpv.duration
                                    focusPolicy: Qt.NoFocus
                                    stepSize: 10
                                    onValueModified: mpv.skipEDLength = value;
                                }
                                AppCheckBox {
                                    id: skipEDCheckBox
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.preferredWidth: 0.25
                                    focusPolicy: Qt.NoFocus
                                    checked: mpv.skipED
                                    onToggled: mpv.skipED = !mpv.skipED;
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 5
                                Text { text: "Auto Skip"; color: "#E5E7EB"; Layout.fillWidth: true; Layout.preferredWidth: 0.25; Layout.fillHeight: true; verticalAlignment: Text.AlignVCenter }
                                AppCheckBox {
                                    id: autoSkipCheckBox
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 0.25
                                    Layout.fillHeight: true
                                    focusPolicy: Qt.NoFocus
                                    checked: mpv.skipED && mpv.skipOP
                                    onToggled: {
                                        const newVal = !(mpv.skipED && mpv.skipOP)
                                        mpv.skipED = newVal
                                        mpv.skipOP = newVal
                                    }
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Enable skip both OP and ED")
                                }
                            }

                        }
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
            cursorShape: Globals.pipMode ? Qt.ArrowCursor : controlBar.visible ? Qt.ArrowCursor : Qt.BlankCursor

            onPressed: (mouse) => {
                           if (mouse.button === Qt.LeftButton && Globals.pipMode) {
                               pressPos = Qt.point(mouse.x, mouse.y)
                               pipDragging = false
                           }
                       }
            onDoubleClicked: {
                if (Globals.pipMode) Globals.togglePip()
                else Globals.toggleFullscreen()
            }
            onClicked: (mouse) => {
                           if (mouse.button === Qt.RightButton)
                           contextMenu.popup()

                       }
            onPositionChanged: (mouse) => {
                                   mpv.peak()
                                   if (Globals.pipMode && mouseArea.pressed && !pipDragging) {
                                       var dx = mouse.x - pressPos.x
                                       var dy = mouse.y - pressPos.y
                                       if (Math.abs(dx) > dragThreshold || Math.abs(dy) > dragThreshold) {
                                           pipDragging = true
                                           Globals.root.startSystemMove()
                                       }
                                   }
                               }
            onCanceled: pipDragging = false
            onReleased: (mouse) => {
                            if (Globals.pipMode && pipDragging) {
                                Globals.root.ensureFullyVisibleOnScreen(); pipDragging = false
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

        ServerListPopup {
            id: serverListPopup
            anchors.centerIn: parent
            width: mpvPage.width / 2.1
            height: mpvPage.height / 2.5
            visible: false
            onClosed: mpvPage.forceActiveFocus()
            function toggle() {
                if (Globals.pipMode) {Globals.togglePip(); return}
                if (opened)
                    close()
                else {
                    open()
                    settingsPopup.close()
                }
                mpvPage.forceActiveFocus()
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
            onSettingsButtonClicked: settingsPopup.toggle(1)
            onCaptionButtonClicked: mpv.subVisible = !mpv.subVisible
            onStopButtonClicked: mpv.stop()

            // Popup {
            //     id: volumePopup
            //     width: 40
            //     height: 120

            //     Slider {
            //         id: volumeSlider
            //         anchors.fill: parent
            //         from: 0
            //         to: 100
            //         value: 50
            //         stepSize: 1
            //         snapMode: Slider.SnapAlways
            //         orientation: Qt.Vertical
            //     }
            // }
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




    PlayListSideBar {
        id: playlistBar
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: parent.width * 0.2
        visible: false
        function toggle() {
            if (Globals.pipMode) return
            playlistBar.visible = !playlistBar.visible
            mpvPage.forceActiveFocus()
        }
    }


    FolderDialog {
        id: folderDialog
        currentFolder: "file:///" + App.settings.downloadDir
        onAccepted: {
            App.play.openUrl(folderDialog.selectedFolder, true)
            mpvPage.forceActiveFocus()
        }
    }
    FileDialog {
        id: fileDialog
        currentFolder: "file:///" + App.settings.downloadDir
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
            normalSpeed = mpv.speed
            mpv.setSpeed(mpv.speed * 2)
        } else {
            mpv.setSpeed(normalSpeed)
        }
    }

    Keys.enabled: true
    Keys.onReleased: event => {
                         switch (event.key) {
                             case Qt.Key_Shift:
                             isDoubleSpeed = false
                         }
                     }
    function increaseSpeed(increment) {
        if (isDoubleSpeed) {
            normalSpeed += increment
            mpv.setSpeed(mpv.speed + increment * 2)
        }
        else {
            mpv.setSpeed(mpv.speed + increment)
        }
    }

    Keys.onPressed: event => {
                        if (!visible) return;
                        if (event.modifiers & Qt.ControlModifier)
                        handleCtrlModifiedKeyPress(event);
                        else
                        handleKeyPress(event)
                    }
    function handleKeyPress(event) {
        if (event.modifiers & Qt.AltModifier) return
        switch (event.key) {
        case Qt.Key_V: serverListPopup.toggle(); break
        case Qt.Key_G: settingsPopup.toggle(); break
        case Qt.Key_M: mpv.muted = !mpv.muted; break
        case Qt.Key_Z:
        case Qt.Key_Left: mpv.seek(mpv.time - 5); break
        case Qt.Key_X:
        case Qt.Key_Right: mpv.seek(mpv.time + 5); break
        case Qt.Key_Tab:
        case Qt.Key_Asterisk: App.play.showCurrentItemName(); break
        case Qt.Key_Slash: mpv.peak(); break
        case Qt.Key_E: fileDialog.open(); break
        case Qt.Key_Shift: isDoubleSpeed = true; break
        case Qt.Key_P: playlistBar.toggle(); break
        case Qt.Key_W: playlistBar.visible = !playlistBar.visible; break
        case Qt.Key_Up:
        case Qt.Key_Q: mpv.volume += volumeStep; break
        case Qt.Key_Down:
        case Qt.Key_A: mpv.volume -= volumeStep; break
        case Qt.Key_Space:
        case Qt.Key_Clear: mpv.togglePlayPause(); break
        case Qt.Key_PageUp: App.play.loadNextItem(1); break
        case Qt.Key_Home: App.play.loadNextItem(-1); break
        case Qt.Key_PageDown: mpv.seek(mpv.time + 90); break
        case Qt.Key_End: mpv.seek(mpv.time - 90); break
        case Qt.Key_Plus:
        case Qt.Key_D: increaseSpeed(0.1); break;
        case Qt.Key_Minus:
        case Qt.Key_S: increaseSpeed(-0.1); break;
        case Qt.Key_Escape:
            if (Globals.pipMode) Globals.togglePip()
            else if (Globals.fullscreen) Globals.toggleFullscreen()
            break
        case Qt.Key_C:
            mpv.subVisible = !mpv.subVisible
            mpv.showText(mpv.subVisible ? "Subtitles enabled" : "Subtitles disabled")
            break

        case Qt.Key_R:
            if (mpv.speed > 1.0) mpv.setSpeed(1.0)
            else mpv.setSpeed(2.0)
            break
        case Qt.Key_F:
            if (Globals.pipMode) Globals.togglePip()
            else Globals.toggleFullscreen()
            break
        default: mpv.sendKeyPress(event.text); break
        }
    }

    function handleCtrlModifiedKeyPress(event) {
        switch (event.key) {
        case Qt.Key_Z: mpv.seek(mpv.time - 90); break
        case Qt.Key_X: mpv.seek(mpv.time + 90); break
        case Qt.Key_V: App.play.openUrl("", true); break
        case Qt.Key_R: App.play.reload(); break
        case Qt.Key_A: { playlistBar.visible = false; Globals.togglePip(); break }
        case Qt.Key_C: mpv.copyVideoLink(); break
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
            if (event.modifiers & Qt.ShiftModifier) Qt.openUrlExternally("file:///" + App.settings.downloadDir)
            else folderDialog.open()
            break
        default:
            mpv.sendKeyPress("CTRL+" + event.text)
            break
        }
    }
}
