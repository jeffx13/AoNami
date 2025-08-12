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
    x: (Screen.width - root.width) / 2
    y: (Screen.height - root.height) / 2
    property int animDuration: 1
    property rect targetRect: Qt.rect((Screen.width - 1080) / 2,
                                     (Screen.height - 720) / 2,
                                     1080, 720)
    property double lastX: (Screen.width - root.width) / 2
    property double lastY: (Screen.height - root.height) / 2

    color: "black"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint
    onClosing: App.play.saveProgress()
    property bool  maximised: false
    property bool fullscreen: false
    property bool pipMode: false
    property int fontSizeMultiplier: !maximised ? 1 : 1.5
    property alias resizeAnime: rectAnimation
    property var mpv

    property real libraryLastScrollY: 0
    property string lastSearch: ""

    Component.onCompleted: {
        if (!App.library.loadFile("")) {
            notifierMessage.text = "Failed to load library"
            headerText.text = "Library Error"
            notifier.open()
            notifier.onClosed = root.close()
        }
        App.library.fetchUnwatchedEpisodes(App.library.listType)
        delayedFunctionTimer.start();
    }



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

    function searchShow(query){
        lastSearch = query
        App.explore(query, 1, false)
        sideBar.gotoPage(0)
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

    StackView {
        id:stackView
        visible: true
        anchors{
            top: titleBar.bottom
            left: sideBar.right
            right: parent.right
            bottom: parent.bottom
        }
        initialItem: "qrc:/src/gui/qml/explorer/ExplorerPage.qml"
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
                    return App.explorer.isLoading || App.showManager.isLoading
                case 1:
                    return App.play.isLoading
                }
                return false;
            }

            cancellable: (0 <= sideBar.currentIndex && sideBar.currentIndex <=2)
            //timeoutEnabled: (0 <= sideBar.currentIndex && sideBar.currentIndex <=2)
            onCancelled: {
                switch(sideBar.currentIndex){
                case 0:
                    if (App.explorer.isLoading) App.explorer.cancel()
                    else if(App.showManager.isLoading) App.showManager.cancel()
                    break;
                case 1:
                    if (App.play.isLoading) App.play.cancel()
                    break;
                case 2 :
                    if (App.showManager.isLoading) App.showManager.cancel() //TODO? loading from library
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



    NumberAnimation on targetRect {
        id: rectAnimation
        duration: animDuration
        easing.type: Easing.InOutQuad
        onRunningChanged: if (root.mpv) root.mpv.setIsResizing(running)
    }

    function animateToRect(x, y, w, h) {
        targetRect = Qt.rect(x, y, w, h)
    }

    // keep window bound to targetRect
    onTargetRectChanged: {
        root.width = targetRect.width
        root.height = targetRect.height
        root.x = targetRect.x
        root.y = targetRect.y
    }

    // Maximised
    onMaximisedChanged: {
        if (rectAnimation.running) return
        if (maximised) {
            lastX = root.x
            lastY = root.y
            animateToRect(0, 0, Screen.desktopAvailableWidth, Screen.desktopAvailableHeight)
        } else {
            animateToRect(lastX, lastY, 1080, 720)
        }
    }

    // Fullscreen
    onFullscreenChanged: {
        if (rectAnimation.running) return
        if (fullscreen) {
            if (root.x !== 0 && root.y !== 0) {
                lastX = root.x
                lastY = root.y
            }
            animateToRect(0, 0, Screen.width, Screen.height)
        } else {
            animateToRect(
                        maximised ? 0 : lastX,
                        maximised ? 0 : lastY,
                        maximised ? Screen.desktopAvailableWidth : 1080,
                        maximised ? Screen.desktopAvailableHeight : 720
                        )
        }
    }

    // PiP
    onPipModeChanged: {
        if (rectAnimation.running) return
        if (pipMode) {
            mpvPage.playListSideBar.visible = false
            animateToRect(
                        Screen.desktopAvailableWidth - Screen.width / 3,
                        Screen.desktopAvailableHeight - Screen.height / 2.3,
                        Screen.width / 3,
                        Screen.height / 2.3
                        )
            flags |= Qt.WindowStaysOnTopHint
        } else {
            animateToRect(
                        fullscreen || maximised ? 0 : (Screen.width - 1080) / 2,
                        fullscreen || maximised ? 0 : (Screen.height - 720) / 2,
                        fullscreen ? Screen.width :
                                     maximised ? Screen.desktopAvailableWidth : 1080,
                        fullscreen ? Screen.height :
                                     maximised ? Screen.desktopAvailableHeight : 720
                        )
            flags &= ~Qt.WindowStaysOnTopHint
        }
    }

    function togglePipMode() {
        if (rectAnimation.running) return
        pipMode = !pipMode
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
            target: ErrorDisplayer
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
            let nextPage = sideBar.currentIndex + 1
            if (nextPage === 1 && !App.showManager.currentShow.exists) nextPage++
            nextPage %= Object.keys(sideBar.pages).length
            sideBar.gotoPage(nextPage)
        }
    }

    Shortcut {
        sequence: "Ctrl+Shift+Tab"
        onActivated: {
            let prevPage = sideBar.currentIndex - 1
            if (prevPage === 1 && !App.showManager.currentShow.exists) prevPage--
            sideBar.gotoPage(prevPage < 0 ? Object.keys(sideBar.pages).length - 1 : prevPage)
        }
    }






}

