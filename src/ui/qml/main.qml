import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import AoNami
import "."
import "./Player"
import "./Components"
import "./Pages"

ApplicationWindow {
    id: root
    width: Globals.defaultWidth
    height: Globals.defaultHeight
    x: (Screen.desktopAvailableWidth - width) / 2
    y: (Screen.desktopAvailableHeight - height) / 2

    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint
    onClosing: {
        App.playlist.saveProgress()
        if (!Globals.maximised && !Globals.fullscreen && !Globals.pipMode) saveGeometry()
        App.settings.setString("win/geom", savedX + "," + savedY + "," + savedW + "," + savedH)
        App.settings.setBool("win/max", Globals.maximised)
        App.settings.setString("win/page", "" + Globals.pageIndex)
    }

    color: Globals.pageIndex === 3 ? "#000000" : Theme.background
    Material.theme: Material.Dark
    Material.primary: Theme.background
    Material.accent: Theme.accent
    Material.foreground: Theme.textPrimary

    Binding { target: Theme; property: "name";         value: App.settings.themeName }
    Binding { target: Theme; property: "customAccent"; value: App.settings.accentColor }
    Binding { target: Globals; property: "uiScale";    value: App.settings.uiScale }


    // Sidebar order, which is what Ctrl+Tab follows - stepping the page numbers put History last.
    readonly property var navOrder: [0, 1, 2, 3, 4, 7, 5, 6]

    function stepPage(delta) {
        let at = navOrder.indexOf(Globals.pageIndex)
        if (at < 0) at = 0
        for (let i = 0; i < navOrder.length; ++i) {
            at = (at + delta + navOrder.length) % navOrder.length
            const page = navOrder[at]
            if (page === 1 && !App.showManager.currentShow.exists) continue
            gotoPage(page)
            return
        }
    }

    property real savedX: x
    property real savedY: y
    property real savedW: width
    property real savedH: height

    function saveGeometry() {
        savedX = x
        savedY = y
        savedW = width
        savedH = height
    }

    function applyGeometry(nx, ny, nw, nh) {
        x = nx
        y = ny
        width = nw
        height = nh
    }

    function restoreGeometry() {
        applyGeometry(savedX, savedY, savedW, savedH)
    }

    function toggleMaximised() {
        if (Globals.pipMode || Globals.fullscreen) return

        if (Globals.maximised) {
            Globals.maximised = false
            restoreGeometry()
        } else {
            saveGeometry()
            Globals.maximised = true
            applyGeometry(
                Screen.virtualX,
                Screen.virtualY,
                Screen.desktopAvailableWidth,
                Screen.desktopAvailableHeight
            )
        }
    }

    function toggleFullscreen() {
        if (Globals.pipMode) togglePip()

        if (Globals.fullscreen) {
            Globals.fullscreen = false
            if (Globals.maximised) {
                applyGeometry(
                    Screen.virtualX,
                    Screen.virtualY,
                    Screen.desktopAvailableWidth,
                    Screen.desktopAvailableHeight
                )
            } else {
                restoreGeometry()
            }
        } else {
            if (!Globals.maximised) saveGeometry()
            Globals.fullscreen = true
            applyGeometry(Screen.virtualX, Screen.virtualY, Screen.width, Screen.height)
            raise()
        }
    }

    function togglePip() {
        if (Globals.pipMode) {
            Globals.pipMode = false
            flags = Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint

            if (Globals.fullscreen) {
                applyGeometry(Screen.virtualX, Screen.virtualY, Screen.width, Screen.height)
                raise()
            } else if (Globals.maximised) {
                applyGeometry(
                    Screen.virtualX,
                    Screen.virtualY,
                    Screen.desktopAvailableWidth,
                    Screen.desktopAvailableHeight
                )
            } else {
                restoreGeometry()
            }
        } else {
            if (!Globals.maximised && !Globals.fullscreen) saveGeometry()
            Globals.fullscreen = false
            Globals.maximised = false
            Globals.pipMode = true

            let pw = Math.round(Screen.desktopAvailableWidth * 0.33)
            let ph = Math.round(Screen.desktopAvailableHeight * 0.45)
            flags = Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint | Qt.WindowStaysOnTopHint
            applyGeometry(
                Screen.desktopAvailableWidth - pw,
                Screen.desktopAvailableHeight - ph,
                pw,
                ph
            )
        }
    }

    function ensureFullyVisibleOnScreen() {
        x = Math.max(0, Math.min(x, Screen.desktopAvailableWidth - width))
        y = Math.max(0, Math.min(y, Screen.desktopAvailableHeight - height))
    }

    readonly property bool chromeVisible: !(Globals.pipMode || Globals.fullscreen)

    Component.onCompleted: {
        Globals.root = root

        var g = App.settings.getString("win/geom", "")
        if (g.length > 0) {
            var parts = g.split(",")
            if (parts.length === 4 && Number(parts[2]) > 200 && Number(parts[3]) > 200) {
                applyGeometry(Number(parts[0]), Number(parts[1]), Number(parts[2]), Number(parts[3]))
                saveGeometry()
                ensureFullyVisibleOnScreen()
                if (App.settings.getBool("win/max", false)) toggleMaximised()
            }
        }

        if (App.playlist.playPlaylist(0)) {
            Globals.pageIndex = 3
            history = [3]
        } else {
            var lastPage = Number(App.settings.getString("win/page", "0"))
            if ([2, 4, 5, 6, 7].indexOf(lastPage) >= 0) {
                Globals.pageIndex = lastPage
                history = [lastPage]
            }
            if (!App.explorer.isLoading && App.explorer.count === 0)
                App.explore("", 1, true)
        }

        deferredStartupTimer.start()
    }

    property var history: [0]
    property int historyIndex: 0

    property string nowPlayingTitle: ""
    property string nowPlayingEpisode: ""
    Connections {
        target: App.playlist
        function onCurrentItemChanged() {
            root.nowPlayingTitle = App.playlist.currentShowName()
            root.nowPlayingEpisode = App.playlist.currentItemName().replace(/\s*\n\s*/g, "  ")
        }
    }

    function gotoPage(index, isHistory = false) {
        if (Globals.fullscreen || Globals.pageIndex === index) return
        if (index === 1 && !App.showManager.currentShow.exists) return

        if (index === 3) {
            Globals.mpv.peek(2000)
            mpvPage.forceActiveFocus()
        }

        Globals.pageIndex = index
        if (!isHistory) {
            history.splice(historyIndex + 1)
            history.push(index)
            historyIndex = history.length - 1
        }
    }

    // gotoPage() bails while fullscreen; stepping the index there would desync it from the page.
    function goHistory(delta) {
        if (Globals.fullscreen) return
        const at = historyIndex + delta
        if (at < 0 || at >= history.length) return
        historyIndex = at
        gotoPage(history[at], true)
    }

    function focusCurrentPage() {
        if (Globals.pageIndex === 3) { mpvPage.forceActiveFocus(); return }
        for (let i = 0; i < pageStack.children.length; i++) {
            let c = pageStack.children[i]
            if (c && c.current === true && c.item) { c.item.forceActiveFocus(); return }
        }
    }

    TitleBar {
        id: titleBar
        height: root.chromeVisible ? 44 : 0
        z: 6
        anchors { top: parent.top; left: parent.left; right: parent.right }

        canGoBack: root.historyIndex > 0
        canGoForward: root.historyIndex + 1 < root.history.length
        nowPlayingTitle: root.nowPlayingTitle
        nowPlayingEpisode: root.nowPlayingEpisode

        onMoveRequested: root.startSystemMove()
        onMaximiseToggled: root.toggleMaximised()
        onMinimiseRequested: root.showMinimized()
        onCloseRequested: root.close()
        onHistoryStep: (delta) => root.goHistory(delta)
        onPlayerRequested: root.gotoPage(3)
    }

    Item {
        id: contentArea
        anchors {
            top: titleBar.bottom
            left: parent.left
            leftMargin: root.chromeVisible ? sideBar.width : 0
            right: parent.right
            bottom: parent.bottom
        }
    }

    SideBar {
        id: sideBar
        z: 5
        chromeVisible: root.chromeVisible
        anchors {
            left: parent.left
            top: titleBar.bottom
            bottom: parent.bottom
        }
        onPageRequested: (page) => root.gotoPage(page)
    }


    Timer {
        id: deferredStartupTimer
        interval: 3000
        repeat: false
        onTriggered: App.library.fetchUnwatchedEpisodes(App.library.libraryType)
    }

    // Pages load lazily and stay cached; the player (page 3) is the MpvPage behind this.
    Item {
        z: 1
        anchors.fill: contentArea
        opacity: Globals.pageIndex === 3 ? 0 : 1
        visible: opacity > 0.001
        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

        // Cache the page as one texture during the cross-fade so it isn't re-rendered every frame.
        layer.enabled: opacity > 0.001 && opacity < 0.999

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: Theme.background }
                GradientStop { position: 1.0; color: Theme.bgBottom }
            }
        }

        StackLayout {
            id: pageStack
            anchors.fill: parent

            readonly property var pageOrder: [0, 1, 2, 4, 5, 6, 7]   // 3 = player, handled separately
            currentIndex: Math.max(0, pageOrder.indexOf(Globals.pageIndex))

            Repeater {
                model: [
                    "Pages/ExplorerPage.qml",
                    "Pages/InfoPage.qml",
                    "Pages/LibraryPage.qml",
                    "Pages/DownloadPage.qml",
                    "Pages/LogPage.qml",
                    "Pages/SettingsPage.qml",
                    "Pages/HistoryPage.qml"
                ]
                delegate: Loader {
                    required property int index
                    required property string modelData
                    readonly property bool current: pageStack.currentIndex === index && Globals.pageIndex !== 3
                    active: false
                    source: active ? modelData : ""
                    Component.onCompleted: if (current) active = true
                    onCurrentChanged: {
                        if (!current) return
                        if (!active) active = true
                        else if (item) item.forceActiveFocus()
                    }
                    onLoaded: if (current && item) item.forceActiveFocus()
                }
            }
        }
    }

    LoadingScreen {
        z: 100
        anchors.fill: contentArea
        loading: {
            switch (Globals.pageIndex) {
            case 0: return App.explorer.isLoading || App.showManager.isLoading
            case 1: return App.playlist.isLoading
            case 2: return App.showManager.isLoading
            case 7: return App.showManager.isLoading
            default: return false
            }
        }
        cancellable: Globals.pageIndex >= 0 && Globals.pageIndex <= 2
        onCancelled: {
            switch (Globals.pageIndex) {
            case 0:
                if (App.explorer.isLoading) App.explorer.cancel()
                else if (App.showManager.isLoading) App.showManager.cancel()
                break
            case 1:
                if (App.playlist.isLoading) App.playlist.cancel()
                break
            case 2:
                if (App.showManager.isLoading) App.showManager.cancel()
                break
            }
        }
    }

    MpvPage {
        id: mpvPage
        // Player page and PiP only, or right-clicks elsewhere reach the player's context menu.
        enabled: Globals.pageIndex === 3 || Globals.pipMode
        anchors.fill: contentArea
    }

    QuickSearch {
        id: quickSearch
        anchors.fill: parent
        z: 200
        onSearched: (query) => {
            Globals.lastSearch = query
            App.explore(query, 1, false)
            root.gotoPage(0)
        }
    }

    Notifier {
        id: notifier
        onLogsRequested: root.gotoPage(5)
        onClosed: {
            if (mpvPage.visible) mpvPage.forceActiveFocus()
            else root.focusCurrentPage()
        }
    }

    Image {
        id: debugOverlay
        anchors.fill: parent
        visible: false
        source: "qrc:/AoNami/resources/images/periodic-table.jpg"
    }

    Connections {
        target: UiBridge
        function onErrorOccurred(message, header) { notifier.show(message, header, true) }
        function onInfoOccurred(message, header)  { notifier.show(message, header, false) }
        function onNavigateRequested(page)        { root.gotoPage(page) }
        function onHistoryStepRequested(delta)    { root.goHistory(delta) }
    }

    Shortcut { sequence: "Alt+Right"; onActivated: root.goHistory(1) }
    Shortcut { sequence: "Alt+Left";  onActivated: root.goHistory(-1) }
    Shortcut { sequence: "Ctrl+Tab";       onActivated: root.stepPage(1) }
    Shortcut { sequence: "Ctrl+Shift+Tab"; onActivated: root.stepPage(-1) }

    Shortcut { sequence: "Ctrl+W"; onActivated: root.close() }
    Shortcut {
        sequence: "Ctrl+R"
        enabled: Globals.pageIndex === 1 && App.showManager.currentShow.exists
        onActivated: App.reloadShow()
    }
    Shortcut { sequence: "Ctrl+K"; onActivated: quickSearch.open = !quickSearch.open }
    Shortcut { sequence: "1"; onActivated: root.gotoPage(0) }
    Shortcut { sequence: "2"; onActivated: root.gotoPage(1) }
    Shortcut { sequence: "3"; onActivated: root.gotoPage(2) }
    Shortcut { sequence: "4"; onActivated: root.gotoPage(3) }
    Shortcut { sequence: "5"; onActivated: root.gotoPage(4) }
    Shortcut { sequence: "6"; onActivated: root.gotoPage(5) }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: {
            if (Globals.pipMode) root.togglePip()
            if (Globals.maximised) root.toggleMaximised()
            if (Globals.fullscreen) root.toggleFullscreen()
            debugOverlay.visible = true
            root.lower()
            root.showMinimized()
            if (Globals.mpv) Globals.mpv.pause()
        }
    }
    Shortcut {
        sequence: "Ctrl+Space"
        onActivated: debugOverlay.visible = !debugOverlay.visible
    }
}