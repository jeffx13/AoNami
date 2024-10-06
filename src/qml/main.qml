import Kyokou.App.Main // qmllint disable
import QtQuick
import QtQuick.Window 2.2
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import "./player"
import "./components"
import "."
// qmllint disable unqualified
ApplicationWindow {
    id: root
    width: 1080
    height: 720
    visible: true
    color: "black"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint

    property bool  maximised: false
    property bool fullscreen: false
    property bool pipMode: false
    property int fontSizeMultiplier: !maximised ? 1 : 1.5

    property real searchResultsViewlastScrollY:0
    property real watchListViewLastScrollY: 0
    property string lastSearch: ""
    property double lastX
    property double lastY

    property alias resizeAnime: resizingAnimation
    property var mpv

    TitleBar {
        id:titleBar
        visible: !(root.pipMode || root.fullscreen)
        focus: false
    }

    SideBar {
        id:sideBar
        visible: !(root.pipMode || root.fullscreen)
        anchors{
            left: parent.left
            top:titleBar.bottom
            bottom:parent.bottom
        }
    }

    StackView {
        id:stackView
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
        pushExit:  Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }
        popEnter:  Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to:1
                duration: 200
            }
        }
        popExit:   Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }

        LoadingScreen {
            id: loadingScreen
            z: 10
            loading: {
                switch(sideBar.currentIndex){
                    case 0:
                        return App.explorer.isLoading || App.currentShow.isLoading
                    case 1:
                        return App.play.isLoading
                    case 2 :
                        return App.library.isLoading
                }
                return false;
            }

            cancellable: (0 <= sideBar.currentIndex && sideBar.currentIndex <=2)
            //timeoutEnabled: (0 <= sideBar.currentIndex && sideBar.currentIndex <=2)
            onCancelled: {
                switch(sideBar.currentIndex){
                    case 0:
                        if (App.explorer.isLoading) App.explorer.cancel()
                        else if(App.currentShow.isLoading) App.currentShow.cancel()
                        break;
                    case 1:
                        if (App.play.isLoading) App.play.cancel()
                        break;
                    case 2 :
                        if (App.currentShow.isLoading && App.currentShow.loadingFromLibrary) App.currentShow.cancel()
                        break;
                }

            }


        }
    }

    MpvPage {
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
                elide: Text.ElideRight
                anchors {
                    top: headerText.bottom
                    bottomMargin: 20
                    leftMargin: 20
                    rightMargin: 20
                    left: parent.left
                    right:parent.right
                    bottom: parent.bottom
                }

            }
            Button {
                id: okButton
                text: "OK"
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: notifier.close()
            }
        }

        Connections {
            target: ErrorHandler
            function onShowWarning(msg, header){
                notifierMessage.text = msg
                headerText.text = header
                notifier.open()
            }
        }

        function onPopupClosed() {
            if (mpvPage.visible)
                mpvPage.forceActiveFocus()
            else
                stackView.currentItem.forceActiveFocus()
        }
        onOpened : {
            okButton.forceActiveFocus()
        }
        onClosed: onPopupClosed()
    }

    ParallelAnimation {
        id: resizingAnimation
        property real speed:10000
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
            root.mpv.setIsResizing(running)
        }
    }

    onClosing: App.updateTimeStamp();

    Component.onCompleted: {
        if (!App.library.loadFile("")) {
            notifierMessage.text = "Failed to load library"
            headerText.text = "Library Error"
            notifier.open()
            notifier.onClosed = root.close()
        }
        delayedFunctionTimer.start();
    }
    Timer {
            id: delayedFunctionTimer
            interval: 100  // 100 milliseconds
            repeat: false  // Only trigger once
            onTriggered: {
                if (App.play.tryPlay(0, -1)) {
                    sideBar.gotoPage(3)
                } else {
                    App.explore("", 1, true)
                    mpvPage.visible = false
                }
            }
        }


    onMaximisedChanged: {
        if (resizingAnimation.running) return
        if (maximised) {
            xanime.to = 0
            yanime.to = 0
            if (root.x !== 0 && root.y !== 0) {
                lastX = root.x
                lastY = root.y
            }
            widthanime.to = Screen.desktopAvailableWidth
            heightanime.to = Screen.desktopAvailableHeight
        }
        else {
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

        }
        else {
            xanime.to = maximised ? 0 : lastX
            yanime.to = maximised ? 0 : lastY
            widthanime.to = maximised ? Screen.desktopAvailableWidth : 1080
            heightanime.to = maximised ? Screen.desktopAvailableHeight : 720
        }

        resizingAnimation.running = true

    }

    function togglePipMode() {
        if (resizingAnimation.running) return
        pipMode = !pipMode
    }

    onPipModeChanged: {
        if (resizingAnimation.running) return
        if (pipMode) {
            mpvPage.playListSideBar.visible = false;
            xanime.to = Screen.desktopAvailableWidth - Screen.width/3
            yanime.to = Screen.desktopAvailableHeight - Screen.height/2.3
            // if (root.x !== 0 && root.y !== 0)
            // {
            //     lastX = root.x
            //     lastY = root.y
            // }
            widthanime.to = Screen.width/3
            heightanime.to = Screen.height/2.3
            flags |= Qt.WindowStaysOnTopHint
        }
        else {
            xanime.to = fullscreen || maximised ? 0 : (Screen.width - 1080) / 2
            yanime.to = fullscreen || maximised ? 0 : (Screen.height - 720) / 2
            widthanime.to = fullscreen ? Screen.width : maximised ? Screen.desktopAvailableWidth : 1080
            heightanime.to = fullscreen ? Screen.height : maximised ? Screen.desktopAvailableHeight : 720
            flags &= ~Qt.WindowStaysOnTopHint
        }
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
        onActivated: {
            if (!root.pipMode) {
                root.close()
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
        sequence: "Ctrl+Q"
        onActivated:
        {
            root.lower()
            root.showMinimized()
            if (root.pipMode) root.pipMode = false
            // if (playerFillWindow) playerFillWindow = false
            if (root.maximised) maximised = false
            if (root.fullscreen) fullscreen = false
            lol.visible = true
            mpv.pause()
        }
    }

    Shortcut{
        sequence: "Ctrl+Space"
        onActivated:
        {
            lol.visible = !lol.visible
        }
    }



    property list<int> history: [0]
    property int historyIndex: 0
    Shortcut {
        sequence: "Alt+Right"
        onActivated: {
            if (historyIndex + 1 < history.length) {
                historyIndex++
                sideBar.gotoPage(history[historyIndex], true)
            }
        }
    }

    Shortcut {
        sequence: "Alt+Left"
        onActivated: {
            if (historyIndex > 0) {
                historyIndex--
                sideBar.gotoPage(history[historyIndex], true)
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+Tab"
        onActivated: {
            let nextPage = (sideBar.currentIndex + 1) % Object.keys(sideBar.pages).length
            sideBar.gotoPage(nextPage)
        }
    }

    Shortcut {
        sequence: "Ctrl+Shift+Tab"
        onActivated: {
            let prevPage = sideBar.currentIndex - 1
            if (prevPage === 1 && !App.currentShow.exists) prevPage--
            sideBar.gotoPage(prevPage < 0 ? Object.keys(sideBar.pages).length - 1 : prevPage)
        }
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

