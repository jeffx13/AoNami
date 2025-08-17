import Kyokou.App.Main // qmllint disable
import QtQuick
import QtQuick.Window 2.2
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts
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
    property double lastX: x
        property double lastY: y
 
     color: "#0B1220"
     Material.theme: Material.Dark
     Material.primary: "#0B1220"
     Material.accent: "#4E5BF2"
     Material.foreground: "#E5E7EB"
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
        // if (!App.library.loadFile("")) {
        //     notifierMessage.text = "Failed to load library"
        //     headerText.text = "Library Error"
        //     notifier.open()
        // }
        App.library.fetchUnwatchedEpisodes(App.library.libraryType)
        delayedFunctionTimer.start();
    }

    function searchShow(query){
        lastSearch = query
        App.explore(query, 1, false)
        sideBar.gotoPage(0)
    }


    Rectangle {
        id: titleBar
        visible: !(root.pipMode || root.fullscreen)
        focus: false
        color: "#0F172A"
        gradient: Gradient {
                GradientStop { position: 0.0; color: "#0B1220" }
                GradientStop { position: 0.5; color: "#0F172A" }
                GradientStop { position: 1.0; color: "#111827" }
            }

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 35

        MouseArea {
            property var clickPos
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onPressed: (mouse)=>{
                           clickPos  = Qt.point(mouse.x,mouse.y)
                       }
            onPositionChanged: (mouse)=> {
                                   if (!root.maximised && clickPos !== null){
                                       root.x = App.cursor.pos().x - clickPos.x
                                       root.y = App.cursor.pos().y - clickPos.y
                                   }
                               }
            onDoubleClicked: {
                root.maximised = !root.maximised
                clickPos = null
            }

        }

        Button  {
            id: closeButton
            width: 14
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 8
            background: Rectangle { color:  "#fa564d"; radius: 7; anchors.fill: parent; border.color: "#ffffff33"; border.width: 1 }
            onClicked: root.close()
            focusPolicy: Qt.NoFocus
        }

        Button {
            id: maxButton
            width: 14
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: closeButton.left
            anchors.rightMargin: 6
            background: Rectangle { color: "#ffbf39"; radius: 7; anchors.fill: parent; border.color: "#ffffff33"; border.width: 1 }
            onClicked: root.maximised = !root.maximised
            focusPolicy: Qt.NoFocus

        }

        Button {
            id: minButton
            width: 14
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: maxButton.left
            anchors.rightMargin: 6
            background: Rectangle { color: "#53cb43"; radius: 7; anchors.fill: parent; border.color: "#ffffff33"; border.width: 1 }
            onClicked: {
                root.showMinimized()
            }
            focusPolicy: Qt.NoFocus
        }

    }

    Rectangle {
        id: sideBar
                gradient: Gradient {
            GradientStop { position: 0.0; color: "#0B1220" }
            GradientStop { position: 0.5; color: "#0E162B" }
            GradientStop { position: 1.0; color: "#101934" }
        }
 
        width: 56
        height: parent.height
        visible: !(root.pipMode || root.fullscreen)
        anchors{
            left: parent.left
            top:titleBar.bottom
            bottom:parent.bottom
        }


        property int currentIndex: 0
        Connections{
            target: App.showManager
            function onShowChanged(){
                sideBar.gotoPage(1)
            }
        }
        Connections{
            target: App.play
            function onAboutToPlay(){
                sideBar.gotoPage(3);
            }
        }


        property var pages: {
            0: "explorer/ExplorerPage.qml",
            1: "info/InfoPage.qml",
            2: "library/LibraryPage.qml",
            3: "player/MpvPage.qml",
            4: "download/DownloadPage.qml",
            5: "log.qml",
            // 6: "settings.qml"
        }

        function gotoPage(index, isHistory = false){
            if (fullscreen || currentIndex === index) return;
            switch(index) {
            case 1:
                if (!App.showManager.currentShow.exists) return;
                break;
            case 3:
                mpv.peak(2000)
                mpvPage.forceActiveFocus()
                mpvPage.visible = true
                stackView.visible = false
                loadingScreen.parent = mpvPage
                break;
            }

            if (index !== 3) {
                stackView.visible = true
                mpvPage.visible = false
                loadingScreen.parent = stackView
                stackView.replace(pages[index])
            }

            currentIndex = index
            if (!isHistory) {
                // remove all pages after current index of history
                history.splice(historyIndex + 1)
                history.push(index)
                historyIndex = history.length - 1
            }

        }

        ColumnLayout {
            height: sideBar.height
            spacing: 5
            ImageButton {
                image: selected ? "qrc:/resources/images/search_selected.png" :"qrc:/resources/images/search.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: {
                    sideBar.gotoPage(0)
                }
                selected: sideBar.currentIndex === 0
            }

            ImageButton {
                id: detailsPageButton
                enabled: App.showManager.currentShow.exists
                image: selected ? "qrc:/resources/images/details_selected.png" : "qrc:/resources/images/details.png"
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: sideBar.gotoPage(1)
                selected: sideBar.currentIndex == 1
            }

            ImageButton {
                id:libraryPageButton
                image: selected ? "qrc:/resources/images/library_selected.png" :"qrc:/resources/images/library.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width

                onClicked: sideBar.gotoPage(2)
                selected: sideBar.currentIndex === 2
            }

            ImageButton {
                id:playerPageButton
                image: selected ? "qrc:/resources/images/tv_selected.png" :"qrc:/resources/images/tv.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: sideBar.gotoPage(3)
                selected: sideBar.currentIndex === 3
            }

            ImageButton {
                id: downloadPageButton
                image: selected ? "qrc:/resources/images/download_selected.png" :"qrc:/resources/images/download.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: sideBar.gotoPage(4)
                selected: sideBar.currentIndex === 4
            }
            ImageButton {
                id: logPageButton
                image: selected ? "qrc:/resources/images/log_selected.png" :"qrc:/resources/images/log.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: sideBar.gotoPage(5)
                selected: sideBar.currentIndex === 5
            }

            // AnimatedImage {
            //     source: "qrc:/resources/gifs/basketball.gif"
            //     Layout.preferredWidth: sideBar.width
            //     Layout.preferredHeight: sideBar.width
            //     // MouseArea {cursorShape: Qt.PointingHandCursor}
            // }
            Rectangle {
                property string orientation: "vertical"
                color: "transparent"

                Layout.fillWidth: orientation == "horizontal"
                Layout.fillHeight: orientation == "vertical"
            }
        }
    }

    Timer {
        id: delayedFunctionTimer
        interval: 100
        repeat: false
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
            color: "#0B1220"
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
        onActivated: root.close()
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
        sequence: "6"
        onActivated: sideBar.gotoPage(5)
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated:
        {
            root.lower()
            root.showMinimized()
            if (root.pipMode) root.pipMode = false
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

