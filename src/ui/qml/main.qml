import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import AoNami
import "."
import "./Player"
import "./Components"
import "./Pages"

ApplicationWindow {
    id: root
    width: 1080
    height: 720
    x: (Screen.desktopAvailableWidth - root.width) / 2
    y: (Screen.desktopAvailableHeight - root.height) / 2
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint
    onClosing: App.play.saveProgress()

    // Theme
    color: "#0B1220"
    Material.theme: Material.Dark
    Material.primary: "#0B1220"
    Material.accent: "#4E5BF2"
    Material.foreground: "#E5E7EB"

    property var pages: {
        0: "Pages/ExplorerPage.qml",
        1: "Pages/InfoPage.qml",
        2: "Pages/LibraryPage.qml",
        3: "",
        4: "Pages/DownloadPage.qml",
        5: "Pages/LogPage.qml",
        6: "Pages/SettingsPage.qml"
    }

    // Window state properties
    property int animDuration: 100

    Component.onCompleted: {
        Globals.root = root
        App.library.fetchUnwatchedEpisodes(App.library.libraryType)
        delayedFunctionTimer.start()
    }

    // History properties
    property list<int> history: [0]
    property int historyIndex: 0
    // Navigation functions
    function gotoPage(index, isHistory = false) {
        if (Globals.fullscreen || Globals.pageIndex === index) return
        switch (index) {
        case 1:
            if (!App.showManager.currentShow.exists) return
            break
        case 3:
            Globals.mpv.peak(2000)
            mpvPage.forceActiveFocus()
            mpvPage.visible = true
            stackView.visible = false
            loadingScreen.parent = mpvPage
            break
        }

        if (index !== 3) {
            stackView.visible = true
            mpvPage.visible = false
            loadingScreen.parent = stackView
            stackView.replace(pages[index])
        }

        Globals.pageIndex = index
        if (!isHistory) {
            // Remove all pages after current index of history
            history.splice(historyIndex + 1)
            history.push(index)
            historyIndex = history.length - 1
        }
    }

    // Animate geometry changes
    Behavior on x { NumberAnimation { duration: animDuration; easing.type: Easing.InOutCubic } }
    Behavior on y { NumberAnimation { duration: animDuration; easing.type: Easing.InOutCubic } }
    Behavior on width { NumberAnimation { duration: animDuration; easing.type: Easing.InOutCubic } }
    Behavior on height { NumberAnimation { duration: animDuration; easing.type: Easing.InOutCubic } }

    // Window state functions
    function toggleMaximised() {
        Globals.maximised = !Globals.maximised
        if (Globals.maximised) {
            showMaximized()
        } else {
            showNormal()
            width = 1080
            height = 720
            x = (Screen.desktopAvailableWidth - 1080) / 2
            y = (Screen.desktopAvailableHeight - 720) / 2
        }
    }

    function toggleFullscreen() {
        Globals.fullscreen = !Globals.fullscreen
        if (Globals.pipMode) {
            togglePip() // Turn off PiP
        }

        if (Globals.fullscreen || Globals.maximised) {
            showMaximized()
        } else if (!Globals.maximised) {
            showNormal()
            width = 1080
            height = 720
            x = (Screen.desktopAvailableWidth - 1080) / 2
            y = (Screen.desktopAvailableHeight - 720) / 2
        }
    }

    function togglePip() {
        Globals.pipMode = !Globals.pipMode
        if (Globals.pipMode) {
            showNormal()
            flags |= Qt.WindowStaysOnTopHint
            var pipW = Math.round(Screen.desktopAvailableWidth * 0.33)
            var pipH = Math.round(Screen.desktopAvailableHeight * 0.45)
            x = Screen.desktopAvailableWidth - pipW
            y = Screen.desktopAvailableHeight - pipH
            width = pipW
            height = pipH
        } else {
            // Turn off PiP
            flags &= ~Qt.WindowStaysOnTopHint
            if (Globals.fullscreen || Globals.maximised) {
                animDuration = 0
                width = 1080
                height = 720
                x = (Screen.desktopAvailableWidth - 1080) / 2
                y = (Screen.desktopAvailableHeight - 720) / 2
                animDuration = 100
                showMaximized()
            } else {
                width = 1080
                height = 720
                x = (Screen.desktopAvailableWidth - 1080) / 2
                y = (Screen.desktopAvailableHeight - 720) / 2
            }
        }
    }

    function ensureFullyVisibleOnScreen() {
        var maxX = Screen.desktopAvailableWidth - width
        var maxY = Screen.desktopAvailableHeight - height
        if (x < 0) x = 0
        else if (x > maxX) x = maxX
        if (y < 0) y = 0
        else if (y > maxY) y = maxY
    }

    // Title bar
    Rectangle {
        id: titleBar
        visible: !(Globals.pipMode || Globals.fullscreen)
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
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onPressed: root.startSystemMove()
            onDoubleClicked: toggleMaximised()
        }

        // Window control buttons
        Button {
            id: closeButton
            width: 14
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 8
            background: Rectangle {
                color: "#fa564d"
                radius: 7
                anchors.fill: parent
                border.color: "#ffffff33"
                border.width: 1
            }
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
            background: Rectangle {
                color: "#ffbf39"
                radius: 7
                anchors.fill: parent
                border.color: "#ffffff33"
                border.width: 1
            }
            onClicked: toggleMaximised()
            focusPolicy: Qt.NoFocus
        }

        Button {
            id: minButton
            width: 14
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: maxButton.left
            anchors.rightMargin: 6
            background: Rectangle {
                color: "#53cb43"
                radius: 7
                anchors.fill: parent
                border.color: "#ffffff33"
                border.width: 1
            }
            onClicked: showMinimized()
            focusPolicy: Qt.NoFocus
        }


    }

    // Sidebar
    Rectangle {
        id: sideBar
        focus: false
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0B1220" }
            GradientStop { position: 0.5; color: "#0E162B" }
            GradientStop { position: 1.0; color: "#101934" }
        }
        width: 56
        height: parent.height
        visible: !(Globals.pipMode || Globals.fullscreen)

        anchors {
            left: parent.left
            top: titleBar.bottom
            bottom: parent.bottom
        }

        ColumnLayout {
            anchors.fill: parent
            height: sideBar.height
            spacing: 5

            ImageButton {
                image: selected ? "qrc:/AoNami/resources/images/search_selected.png" : "qrc:/AoNami/resources/images/search.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: gotoPage(0)
                selected: Globals.pageIndex === 0
            }

            ImageButton {
                id: detailsPageButton
                enabled: App.showManager.currentShow.exists
                image: selected ? "qrc:/AoNami/resources/images/details_selected.png" : "qrc:/AoNami/resources/images/details.png"
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: gotoPage(1)
                selected: Globals.pageIndex == 1
            }

            ImageButton {
                id: libraryPageButton
                image: selected ? "qrc:/AoNami/resources/images/library_selected.png" : "qrc:/AoNami/resources/images/library.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: gotoPage(2)
                selected: Globals.pageIndex === 2
            }

            ImageButton {
                id: playerPageButton
                image: selected ? "qrc:/AoNami/resources/images/tv_selected.png" : "qrc:/AoNami/resources/images/tv.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: gotoPage(3)
                selected: Globals.pageIndex === 3
            }

            ImageButton {
                id: downloadPageButton
                image: selected ? "qrc:/AoNami/resources/images/download_selected.png" : "qrc:/AoNami/resources/images/download.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: gotoPage(4)
                selected: Globals.pageIndex === 4
            }

            ImageButton {
                id: logPageButton
                image: selected ? "qrc:/AoNami/resources/images/log_selected.png" : "qrc:/AoNami/resources/images/log.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: gotoPage(5)
                selected: Globals.pageIndex === 5
            }

            ImageButton {
                id: settingsPageButton
                image: selected ? "qrc:/AoNami/resources/images/settings_selected.png" : "qrc:/AoNami/resources/images/settings.png"
                Layout.preferredWidth: sideBar.width
                Layout.preferredHeight: sideBar.width
                onClicked: gotoPage(6)
                selected: Globals.pageIndex === 6
            }

            Item {
                // Spacer
                Layout.fillHeight: true
            }

            // AnimatedImage {
            //     source: "qrc:/AoNami/resources/gifs/basketball.gif"
            //     Layout.preferredWidth: sideBar.width
            //     Layout.preferredHeight: sideBar.width
            //     playing: hh.hovered
            //     HoverHandler { id: hh }
            // }
        }
    }

    Timer {
        id: delayedFunctionTimer
        interval: 100
        repeat: false
        onTriggered: {
            if (App.play.playPlaylist(0))
                gotoPage(3)
            else App.explore("", 1, true)
        }
    }

    // Main content area
    StackView {
        id: stackView
        visible: true
        focus: visible
        anchors {
            top: titleBar.bottom
            left: sideBar.right
            right: parent.right
            bottom: parent.bottom
        }

        initialItem: "Pages/ExplorerPage.qml"
        background: Rectangle { color: "#0B1220" }

        // Transition animations
        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
            }
        }

        pushExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 200
            }
        }

        popEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
            }
        }

        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 200
            }
        }

        LoadingScreen {
            id: loadingScreen
            z: 10
            loading: {
                switch (Globals.pageIndex) {
                case 0:
                    return App.showManager.isLoading
                case 1:
                    return App.play.isLoading
                }
                return false
            }

            cancellable: (0 <= Globals.pageIndex && Globals.pageIndex <= 2)
            onCancelled: {
                switch (Globals.pageIndex) {
                case 0:
                    if (App.explorer.isLoading) App.explorer.cancel()
                    else if (App.showManager.isLoading) App.showManager.cancel()
                    break
                case 1:
                    if (App.play.isLoading) App.play.cancel()
                    break
                case 2:
                    if (App.showManager.isLoading) App.showManager.cancel()
                    break
                }
            }
        }
    }

    // MPV Player page
    MpvPage {
        id: mpvPage
        visible: Globals.pageIndex === 3
        focus: visible
        anchors.fill: (Globals.fullscreen || Globals.pipMode) ? parent : stackView
    }


    // Error notification popup
    Popup {
        id: notifier
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        property int notifierPadding: 20
        width: Math.min(520, Math.round(parent.width * 0.9))
        implicitHeight: column.implicitHeight + notifierPadding * 2
        anchors.centerIn: parent
        scale: 1.0
        opacity: 1.0

        Overlay.modal: Rectangle { color: "#00000099" }

        background: Item {
            id: bgRoot
            implicitWidth: 400
            implicitHeight: 220
            Rectangle {
                id: bgCard
                anchors.fill: parent
                radius: 14
                color: "#0F172A"
                border.color: "#4E5BF233"
                border.width: 1
            }
            DropShadow {
                anchors.fill: bgCard
                source: bgCard
                horizontalOffset: 0
                verticalOffset: 12
                radius: 24
                samples: 32
                color: "#00000088"
                transparentBorder: true
            }
        }

        enter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160; easing.type: Easing.OutCubic }
                NumberAnimation { property: "scale"; from: 0.94; to: 1.0; duration: 180; easing.type: Easing.OutBack }
            }
        }
        exit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 140; easing.type: Easing.InCubic }
                NumberAnimation { property: "scale"; from: 1.0; to: 0.96; duration: 140; easing.type: Easing.InCubic }
            }
        }

        contentItem: ColumnLayout {
            id: column
            spacing: 14
            width: parent ? parent.width - (notifier.notifierPadding * 2) : 400

            RowLayout {
                spacing: 6
                Layout.fillWidth: true
                Text {
                    id: headerText
                    text: "Error"
                    color: "#FCA5A5"
                    font.pixelSize: 18 * Globals.fontSizeMultiplier
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            ScrollView {
                id: messageScroll
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(220, Math.max(80, implicitHeight))
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                Text {
                    id: notifierMessage
                    text: "An error has occurred."
                    wrapMode: Text.Wrap
                    color: "#E5E7EB"
                    opacity: 0.9
                    font.pixelSize: 14 * Globals.fontSizeMultiplier
                    Layout.fillWidth: true
                }
            }

            Item { Layout.fillHeight: true; visible: false; height: 0 }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                AppButton {
                    id: logsButton
                    text: "View Logs"
                    backgroundDefaultColor: "#374151"
                    contentItemTextColor: "#E5E7EB"
                    cornerRadius: 10
                    fontSize: 16
                    onClicked: { notifier.close(); gotoPage(5) }
                    Layout.alignment: Qt.AlignLeft
                }
                Item { Layout.fillWidth: true }
                AppButton {
                    id: okButton
                    text: "OK"
                    cornerRadius: 10
                    fontSize: 16
                    onClicked: notifier.close()
                }
            }
        }


        Connections {
            target: UiBridge
            function onErrorOccurred(message, header) {
                notifierMessage.text = message
                headerText.text = header
                notifier.open()
            }
            function onNavigateRequested(page) {
                gotoPage(page)
            }
        }

        function onPopupClosed() {
            if (mpvPage.visible)
                mpvPage.forceActiveFocus()
            else
                stackView.currentItem.forceActiveFocus()
        }

        onOpened: okButton.forceActiveFocus()
        onClosed: onPopupClosed()
    }

    // Debug image (hidden)
    Image {
        id: lol
        anchors.fill: parent
        visible: false
        source: "qrc:/AoNami/resources/images/periodic-table.jpg"
    }

    // Navigation shortcuts
    Shortcut {
        sequence: "Alt+Right"
        onActivated: {
            if (historyIndex + 1 < history.length) {
                historyIndex++
                gotoPage(history[historyIndex], true)
            }
        }
    }

    Shortcut {
        sequence: "Alt+Left"
        onActivated: {
            if (historyIndex > 0) {
                historyIndex--
                gotoPage(history[historyIndex], true)
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+Tab"
        onActivated: {
            let nextPage = Globals.pageIndex + 1
            if (nextPage === 1 && !App.showManager.currentShow.exists) nextPage++
            nextPage %= Object.keys(pages).length
            gotoPage(nextPage)
        }
    }

    Shortcut {
        sequence: "Ctrl+Shift+Tab"
        onActivated: {
            let prevPage = Globals.pageIndex - 1
            if (prevPage === 1 && !App.showManager.currentShow.exists) prevPage--
            gotoPage(prevPage < 0 ? Object.keys(pages).length - 1 : prevPage)
        }
    }

    // General shortcuts
    Shortcut {
        sequence: "Ctrl+W"
        onActivated: root.close()
    }

    Shortcut {
        sequence: "1"
        onActivated: gotoPage(0)
    }

    Shortcut {
        sequence: "2"
        onActivated: gotoPage(1)
    }

    Shortcut {
        sequence: "3"
        onActivated: gotoPage(2)
    }

    Shortcut {
        sequence: "4"
        onActivated: gotoPage(3)
    }

    Shortcut {
        sequence: "5"
        onActivated: gotoPage(4)
    }

    Shortcut {
        sequence: "6"
        onActivated: gotoPage(5)
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: {
            if (Globals.pipMode) togglePip()
            if (Globals.maximised) toggleMaximised()
            if (Globals.fullscreen) toggleFullscreen()
            lol.visible = true
            root.lower()
            root.showMinimized()
            if (Globals.mpv) Globals.mpv.pause()
        }
    }

    Shortcut {
        sequence: "Ctrl+Space"
        onActivated: lol.visible = !lol.visible
    }
}
