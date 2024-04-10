import QtQuick
import QtQuick.Window 2.2
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MpvPlayer 1.0
import QtQuick.Controls.Material 2.15
import "./explorer"
import "./info"
import "./player"
import "./library"
import "./download"
import "./components"
import "."

Window {
    id: root
    width: 1080
    height: 720
    visible: true
    color: "black"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint

    property bool maximised: false
    property bool fullscreen: false
    property bool pipMode: false
    property int fontSizeMultiplier: !maximised ? 1 : 1.5

    property real searchResultsViewlastScrollY:0
    property real watchListViewLastScrollY: 0
    property string lastSearch: ""
    property double lastX
    property double lastY

    property alias resizeAnime: resizingAnimation
    property MpvObject mpv



    // Shortcut {
    //     id: test
    //     sequence: "B"
    //     onActivated:
    //     {

    //     }
    // }

    TitleBar {
        z:root.z
        id:titleBar
        visible: !(pipMode || fullscreen)
        focus: false
    }

    SideBar {
        z:root.z
        id:sideBar
        visible: !(pipMode || fullscreen)
        anchors{
            left: parent.left
            top:titleBar.bottom
            bottom:parent.bottom
        }
    }

    StackView {
        id:stackView
        z:root.z
        visible: true
        anchors{
            top: titleBar.bottom
            left: sideBar.right
            right: parent.right
            bottom: parent.bottom
        }
        initialItem: "qrc:/src/qml/explorer/ExplorerPage.qml"
        background: Rectangle{
            color: "black"
        }

        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to:1
                duration: 200
            }
        }
        pushExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }
        popEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to:1
                duration: 200
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }
    }

    MpvPage {
        z:root.z
        id:mpvPage
        visible: true
        anchors.fill: (root.fullscreen || root.pipMode) ? parent : stackView
    }




    Popup {
        id: notifier
        modal: true
        width: parent.width / 3
        height: parent.height / 4
        anchors.centerIn: parent
        contentItem: Rectangle {
            color: "#f2f2f2"
            border.color: "#c2c2c2"
            border.width: 1
            radius: 10
            anchors.centerIn: parent
            Text {
                id: headerText
                text: "Error"
                font.pointSize: 16
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 20
            }
            Text {
                id: notifierMessage
                text: "An error has occurred."
                wrapMode: Text.Wrap
                font.pointSize: 14
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
            Button {
                text: "OK"
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: notifier.close()
            }
        }

        Connections {
            target: errorHandler
            function onShowWarning(msg, header){
                notifierMessage.text = msg
                headerText.text = header
                notifier.open()
            }
        }

        property var onPopupClosed: function() {
            if (mpvPage.visible)
                mpvPage.forceActiveFocus()
            else
                stackView.currentItem.forceActiveFocus()
        }

        onClosed: onPopupClosed()
    }


    Component.onCompleted: {
        if (!app.library.loadFile("")) {
            notifierMessage.text = "Failed to load library"
            headerText.text = "Library Error"
            notifier.open()
            notifier.onPopupClosed = function() {
                root.close()
            }
        }
        setTimeout(() => {
                       if (app.playlist.tryPlay(0, -1)) {
                           sideBar.gotoPage(3)
                       } else {
                           app.latest(1)
                           mpvPage.visible = false
                       }

                   }, 100)
    }



    ParallelAnimation {
        id: resizingAnimation
        property real speed:6666
        SmoothedAnimation {
            id:widthanime
            target: root
            properties: "width";
            to: Screen.desktopAvailableWidth
            velocity: resizingAnimation.speed
        }
        SmoothedAnimation {
            id:heightanime
            target: root
            properties: "height";
            to: Screen.desktopAvailableHeight
            velocity: resizingAnimation.speed
        }
        SmoothedAnimation {
            id:xanime
            target: root
            properties: "x";
            velocity: resizingAnimation.speed
            to: 0
        }
        SmoothedAnimation {
            id:yanime
            target: root
            properties: "y";
            to: 0
            velocity: resizingAnimation.speed
        }

        onRunningChanged: {
            // lags when resizing with mpv playing a video, stop the rendering
            mpv.setIsResizing(running)
        }
    }

    // does not cover the taskbar
    onMaximisedChanged: {
        if (resizingAnimation.running) return
        if (maximised)
        {
            xanime.to = 0
            yanime.to = 0
            if (root.x !== 0 && root.y !== 0) {
                lastX = root.x
                lastY = root.y
            }
            widthanime.to = Screen.desktopAvailableWidth
            heightanime.to = Screen.desktopAvailableHeight
        }
        else
        {
            xanime.to = lastX
            yanime.to = lastY
            widthanime.to = 1080
            heightanime.to = 720
        }
        resizingAnimation.running = true
    }

    onFullscreenChanged: {
        if (resizingAnimation.running) return
        if (fullscreen) {
            xanime.to = 0
            yanime.to = 0
            if (root.x !== 0 && root.y !== 0)
            {
                lastX = root.x
                lastY = root.y
            }
            widthanime.to = Screen.width
            heightanime.to = Screen.height

        } else {
            xanime.to = maximised ? 0 : lastX
            yanime.to = maximised ? 0 : lastY
            widthanime.to = maximised ? Screen.desktopAvailableWidth : 1080
            heightanime.to = maximised ? Screen.desktopAvailableHeight : 720
        }

        resizingAnimation.running = true

    }

    onPipModeChanged: {
        if (resizingAnimation.running) return
        if (pipMode) {
            mpvPage.playListSideBar.visible = false;
            xanime.to = Screen.desktopAvailableWidth - Screen.width/3
            yanime.to = Screen.desktopAvailableHeight - Screen.height/2.3
            if (root.x !== 0 && root.y !== 0)
            {
                lastX = root.x
                lastY = root.y
            }
            widthanime.to = Screen.width/3
            heightanime.to = Screen.height/2.3
            flags |= Qt.WindowStaysOnTopHint
            sideBar.gotoPage(3)
            // playerFillWindow = true
        }
        else {
            xanime.to = fullscreen || maximised ? 0 : lastX
            yanime.to = fullscreen || maximised ? 0 : lastY
            widthanime.to = fullscreen ? Screen.width : maximised ? Screen.desktopAvailableWidth : 1080
            heightanime.to = fullscreen ? Screen.height : maximised ? Screen.desktopAvailableHeight : 720
            flags &= ~Qt.WindowStaysOnTopHint
            // playerFillWindow = fullscreen
        }
        // root.mpvWasPlaying = mpv.state == MpvObject.VIDEO_PLAYING
        // if (root.mpvWasPlaying) mpv.pause()
        resizingAnimation.running = true

    }




    Image {
        id:lol
        anchors.fill: parent
        visible: false
        source: "qrc:/resources/images/periodic-table.jpg"
    }

    Shortcut{
        sequence: "Ctrl+W"
        onActivated:
        {
            if (!pipMode) {
                app.updateTimeStamp()
                Qt.quit()
            }
        }
    }
    Shortcut{
        sequence: "1"
        onActivated: sideBar.gotoPage(0)
    }
    Shortcut{
        sequence: "2"
        onActivated: sideBar.gotoPage(1)
    }
    Shortcut{
        sequence: "3"
        onActivated: sideBar.gotoPage(2)
    }
    Shortcut{
        sequence: "4"
        onActivated: sideBar.gotoPage(3)
    }
    Shortcut {
        sequence: "5"
        onActivated: sideBar.gotoPage(4)
    }

    Shortcut {
        sequence: "Ctrl+Tab"
        onActivated:
        {
            root.lower()
            root.showMinimized()
            if (pipMode) pipMode = false
            // if (playerFillWindow) playerFillWindow = false
            if (maximised) maximised = false
            if (fullscreen) fullscreen = false
            lol.visible = true
            mpv.pause()
        }
    }

    Shortcut{
        sequence: "Ctrl+Q"
        onActivated:
        {
            lol.visible = !lol.visible

        }
    }


    Shortcut {
        sequence: "Ctrl+A"
        onActivated:
        {
            pipMode = !pipMode
        }
    }

    Timer {
        id: callbackTimer
        running: false
        repeat: false

        property var callback

        onTriggered: callback()
    }

    function setTimeout(callback, delay){
        if (callbackTimer.running){
            console.error("nested calls to setTimeout are not supported!");
            return;
        }
        callbackTimer.callback = callback;
        // note: an interval of 0 is directly triggered, so add a little padding
        callbackTimer.interval = delay + 1;
        callbackTimer.running = true;
    }
    // MouseArea {
    //     z:root.z - 1
    //     anchors.fill: parent
    //     acceptedButtons: Qt.ForwardButton | Qt.BackButton
    //     propagateComposedEvents: true
    //     onClicked: (mouse)=>
    //                {
    //                    if (playerFillWindow) return;
    //                    if (mouse.button === Qt.BackButton)
    //                    {
    //                        let nextPage = sideBar.currentPage + 1
    //                        sideBar.gotoPage(nextPage % Object.keys(sideBar.pages).length)
    //                    }
    //                    else
    //                    {
    //                        let prevPage = sideBar.currentPage-1
    //                        sideBar.gotoPage(prevPage < 0 ? Object.keys(sideBar.pages).length-1 : prevPage)
    //                    }
    //                }
    // }


}

