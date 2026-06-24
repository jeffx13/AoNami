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

    color: Theme.background
    Material.theme: Material.Dark
    Material.primary: Theme.background
    Material.accent: Theme.accent
    Material.foreground: Theme.textPrimary

    // Theme + accent persist in settings; changing either re-skins the app live.
    Binding { target: Theme; property: "name";         value: App.settings.themeName }
    Binding { target: Theme; property: "customAccent"; value: App.settings.accentColor }
    Binding { target: Globals; property: "uiScale";    value: App.settings.uiScale }

    readonly property var pages: ({
        0: "Pages/ExplorerPage.qml",
        1: "Pages/InfoPage.qml",
        2: "Pages/LibraryPage.qml",
        3: "Player/MpvPage",
        4: "Pages/DownloadPage.qml",
        5: "Pages/LogPage.qml",
        6: "Pages/SettingsPage.qml"
    })
    readonly property int pageCount: Object.keys(pages).length


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

    function centerDefault() {
        applyGeometry(
            (Screen.desktopAvailableWidth - Globals.defaultWidth) / 2,
            (Screen.desktopAvailableHeight - Globals.defaultHeight) / 2,
            Globals.defaultWidth,
            Globals.defaultHeight
        )
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

        // restore last window geometry
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
            if ([2, 4, 5, 6].indexOf(lastPage) >= 0) {
                Globals.pageIndex = lastPage
                history = [lastPage]
            }
            if (!App.explorer.isLoading && App.searchResultModel.count === 0)
                App.explore("", 1, true)
        }

        deferredStartupTimer.start()
    }

    property var history: [0]
    property int historyIndex: 0

    // Show + episode of whatever is loaded in the player, for the now-playing pill.
    property string nowPlayingTitle: ""
    property string nowPlayingEpisode: ""
    Connections {
        target: App.playlist
        function onCurrentItemChanged() {
            root.nowPlayingTitle = App.playlist.currentShowName()
            root.nowPlayingEpisode = App.playlist.currentItemName()
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

    function focusCurrentPage() {
        if (Globals.pageIndex === 3) { mpvPage.forceActiveFocus(); return }
        for (let i = 0; i < pageStack.children.length; i++) {
            let c = pageStack.children[i]
            if (c && c.current === true && c.item) { c.item.forceActiveFocus(); return }
        }
    }

    Rectangle {
        id: titleBar
        visible: height > 0
        focus: false
        height: chromeVisible ? 44 : 0
        z: 6
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.surface }
            GradientStop { position: 1.0; color: Theme.surfaceDeep }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.border
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onPressed: root.startSystemMove()
            onDoubleClicked: toggleMaximised()
        }

        // Left: back / forward + wordmark
        Row {
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 10 }
            spacing: 2

            Repeater {
                model: [{ glyph: "\u2039", forward: false }, { glyph: "\u203A", forward: true }]
                delegate: Rectangle {
                    required property var modelData
                    width: 30; height: 30; radius: 8
                    readonly property bool canGo: modelData.forward ? (root.historyIndex + 1 < root.history.length)
                                                                    : (root.historyIndex > 0)
                    color: navArea.containsMouse && canGo ? Qt.alpha(Theme.accent, 0.14) : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: modelData.glyph
                        font.pixelSize: 19
                        color: parent.canGo ? Theme.textSecondary : Theme.textMuted
                        opacity: parent.canGo ? 1.0 : 0.4
                    }
                    MouseArea {
                        id: navArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: parent.canGo ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: {
                            if (!parent.canGo) return
                            if (modelData.forward) root.historyIndex++
                            else                   root.historyIndex--
                            gotoPage(root.history[root.historyIndex], true)
                        }
                    }
                }
            }

        }

        // Center: now-playing pill - transport controls + clipped progress fill
        Rectangle {
            id: npPill
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            visible: root.nowPlayingTitle !== "" && Globals.pageIndex !== 3
            height: 32
            width: Math.min(npRow.implicitWidth + 24, parent.width - 360)
            radius: height / 2
            clip: true
            color: Qt.alpha(Theme.accent, 0.12)
            border.color: Theme.accent; border.width: 1

            readonly property real progress: Globals.mpv && Globals.mpv.duration > 0
                                             ? Globals.mpv.time / Globals.mpv.duration : 0

            component NpBtn: Rectangle {
                property string glyph: ""
                signal tapped()
                anchors.verticalCenter: parent.verticalCenter
                width: 24; height: 24; radius: 12
                color: nbArea.containsMouse ? Qt.alpha(Theme.accent, 0.30) : "transparent"
                Behavior on color { ColorAnimation { duration: 120 } }
                Text { anchors.centerIn: parent; text: glyph; color: Theme.textPrimary; font.pixelSize: 12 }
                MouseArea { id: nbArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: tapped() }
            }

            // progress fill: clipped by the pill so it follows the rounded shape (no overlap)
            Rectangle {
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                width: parent.width * npPill.progress
                Behavior on width { NumberAnimation { duration: 250 } }
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Qt.alpha("#22D3EE", 0.10) }
                    GradientStop { position: 1.0; color: Qt.alpha("#A855F7", 0.30) }
                }
                Rectangle {   // playhead
                    anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
                    width: 2; color: "#C4B5FD"
                    opacity: npPill.progress > 0.01 ? 0.9 : 0
                }
            }

            Row {
                id: npRow
                anchors.centerIn: parent
                spacing: 5
                NpBtn { glyph: "\u23EE"; onTapped: App.playlist.loadNextItem(-1) }
                NpBtn {
                    glyph: (Globals.mpv && Globals.mpv.state === 1) ? "\u23F8" : "\u25B6"
                    onTapped: { if (!Globals.mpv) return; if (Globals.mpv.state === 1) Globals.mpv.pause(); else Globals.mpv.play() }
                }
                NpBtn { glyph: "\u23ED"; onTapped: App.playlist.loadNextItem(1) }
                Rectangle { anchors.verticalCenter: parent.verticalCenter; width: 1; height: 16; color: Qt.alpha(Theme.accent, 0.45) }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.nowPlayingTitle
                    color: Theme.textPrimary
                    font.pixelSize: Globals.sp(15)
                    elide: Text.ElideRight
                    width: Math.min(implicitWidth, 220)
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: gotoPage(3) }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.nowPlayingEpisode !== ""
                    text: "\u00B7  " + root.nowPlayingEpisode
                    color: Theme.textMuted
                    font.pixelSize: Globals.sp(14)
                    elide: Text.ElideRight
                    width: Math.min(implicitWidth, 150)
                }
            }
        }

        // Right: window controls
        Row {
            id: trafficLights
            anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 12 }
            spacing: 8
            layoutDirection: Qt.RightToLeft
            HoverHandler { id: groupHandler }
            Repeater {
                model: [
                    { dotColor: "#ff5f57", groupColor: "#fa564d", borderColor: "#e0443e", icon: "\u2715", iconSize: 9  },
                    { dotColor: "#febc2e", groupColor: "#ffbf39", borderColor: "#dfa020",                 iconSize: 7  },
                    { dotColor: "#28c840", groupColor: "#53cb43", borderColor: "#1aab29", icon: "\u2212", iconSize: 11 }
                ]
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    width: 14; height: 14; radius: 7
                    HoverHandler { id: dotHover }
                    color: dotHover.hovered    ? modelData.dotColor
                         : groupHandler.hovered ? modelData.groupColor
                         : Theme.surfaceAlt
                    border.color: dotHover.hovered ? modelData.borderColor : Theme.border
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        visible: groupHandler.hovered
                        text: index === 1 ? (Globals.maximised ? "\u274F" : "\u25FB") : (modelData.icon ?? "")
                        font.pixelSize: modelData.iconSize
                        color: "#00000099"
                    }
                    TapHandler {
                        onTapped: {
                            if (index === 0) root.close()
                            else if (index === 1) toggleMaximised()
                            else root.showMinimized()
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: sideBar
        visible: width > 0
        focus: false
        z: 5
        readonly property int rail: 56
        property bool locked: App.settings.getBool("ui/sidebarLocked", false)
        property bool lockedExpanded: App.settings.getBool("ui/sidebarLockedExpanded", false)
        property bool hoverExpanded: false
        readonly property bool expanded: locked ? lockedExpanded : hoverExpanded
        width: chromeVisible ? (expanded ? 160 : rail) : 0
        function requestExpand() { if (!locked) expandTimer.restart() }
        anchors {
            left: parent.left
            top: titleBar.bottom
            bottom: parent.bottom
        }
        clip: true
        Behavior on width { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.surface }
            GradientStop { position: 1.0; color: Theme.surfaceDeep }
        }

        Rectangle {   // right edge
            anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
            width: 1; color: Theme.border
        }

        HoverHandler { id: hoverHandler; onHoveredChanged: if (!hovered) { expandTimer.stop(); sideBar.hoverExpanded = false } }
        Timer { id: expandTimer; interval: 200; onTriggered: if (!sideBar.locked) sideBar.hoverExpanded = true }

        component SideItem: Item {
            id: si
            property int page: 0
            property string icon: ""
            property string selectedIcon: ""
            property string label: ""
            property bool needsShow: false
            width: parent ? parent.width : 0
            height: 52
            readonly property bool isSelected: Globals.pageIndex === page
            readonly property bool isEnabled: needsShow ? App.showManager.currentShow.exists : true

            Rectangle {   // selection / hover box - always aligned to this item
                anchors.fill: parent
                anchors.leftMargin: 6; anchors.rightMargin: 6
                anchors.topMargin: 4; anchors.bottomMargin: 4
                radius: 10
                color: si.isSelected     ? Qt.alpha(Theme.accent, 0.15)
                     : itemHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                border.color: si.isSelected ? Theme.accent : "transparent"
                border.width: si.isSelected ? 1 : 0
                Behavior on color { ColorAnimation { duration: 140 } }
            }
            Image {
                source: "qrc:/AoNami/resources/images/" + (si.isSelected ? si.selectedIcon : si.icon) + ".png"
                width: 38; height: 38
                fillMode: Image.PreserveAspectFit
                x: (sideBar.rail - width) / 2
                anchors.verticalCenter: parent.verticalCenter
                opacity: si.isEnabled ? 1.0 : 0.35
            }
            Text {
                anchors { left: parent.left; leftMargin: sideBar.rail; right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                text: si.label
                color: si.isSelected ? Theme.textPrimary : Theme.textSecondary
                font.pixelSize: Globals.sp(18)
                elide: Text.ElideRight
                opacity: sideBar.expanded ? (si.isEnabled ? 1.0 : 0.4) : 0.0
                visible: opacity > 0.01
                Behavior on opacity { NumberAnimation { duration: 120 } }
            }
            HoverHandler { id: itemHover; onHoveredChanged: if (hovered) sideBar.requestExpand() }
            MouseArea {
                anchors.fill: parent
                cursorShape: si.isEnabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                onClicked: if (si.isEnabled) gotoPage(si.page)
            }
            AppToolTip { text: si.label; visible: itemHover.hovered && !sideBar.expanded }
        }

        Column {
            anchors { left: parent.left; right: parent.right; top: parent.top; topMargin: 10 }
            spacing: 2

            Item {
                width: parent.width
                height: 40
                Rectangle {
                    width: 34; height: 34; radius: 9
                    anchors.verticalCenter: parent.verticalCenter
                    x: (sideBar.rail - width) / 2
                    color: sideBar.locked ? Qt.alpha(Theme.accent, 0.18) : (pinHover.hovered ? Qt.rgba(1, 1, 1, 0.06) : "transparent")
                    border.color: sideBar.locked ? Theme.accent : "transparent"
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "📌"; font.pixelSize: 15; opacity: sideBar.locked ? 1.0 : 0.5 }
                    HoverHandler { id: pinHover }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (sideBar.locked) {
                                sideBar.locked = false
                                App.settings.setBool("ui/sidebarLocked", false)
                            } else {
                                sideBar.lockedExpanded = sideBar.expanded
                                sideBar.locked = true
                                App.settings.setBool("ui/sidebarLockedExpanded", sideBar.lockedExpanded)
                                App.settings.setBool("ui/sidebarLocked", true)
                            }
                        }
                    }
                    AppToolTip { text: sideBar.locked ? qsTr("Unlock sidebar") : (sideBar.expanded ? qsTr("Lock open") : qsTr("Lock collapsed")); visible: pinHover.hovered }
                }
            }

            SideItem { page: 0; icon: "search";   selectedIcon: "search_selected";   label: "Explore" }
            SideItem { page: 1; icon: "details";  selectedIcon: "details_selected";  label: "Details"; needsShow: true }
            SideItem { page: 2; icon: "library";  selectedIcon: "library_selected";  label: "Library" }
            SideItem { page: 3; icon: "tv";       selectedIcon: "tv_selected";       label: "Player" }
            SideItem { page: 4; icon: "download"; selectedIcon: "download_selected"; label: "Downloads" }
        }

        Column {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: 10 }
            spacing: 2
            SideItem { page: 5; icon: "log";      selectedIcon: "log_selected";      label: "Logs" }
            SideItem { page: 6; icon: "settings"; selectedIcon: "settings_selected"; label: "Settings" }
        }
    }

    Timer {
        id: deferredStartupTimer
        interval: 3000
        repeat: false
        onTriggered: App.library.fetchUnwatchedEpisodes(App.library.libraryType)
    }

    // Pages load lazily and stay cached; the player (page 3) is the MpvPage behind this.
    Item {
        id: pageContainer
        z: 1
        anchors {
            top: titleBar.bottom
            left: parent.left
            leftMargin: chromeVisible ? sideBar.width : 0
            right: parent.right
            bottom: parent.bottom
        }
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

            readonly property var pageOrder: [0, 1, 2, 4, 5, 6]   // 3 = player, handled separately
            currentIndex: Math.max(0, pageOrder.indexOf(Globals.pageIndex))

            Repeater {
                model: [
                    "Pages/ExplorerPage.qml",
                    "Pages/InfoPage.qml",
                    "Pages/LibraryPage.qml",
                    "Pages/DownloadPage.qml",
                    "Pages/LogPage.qml",
                    "Pages/SettingsPage.qml"
                ]
                delegate: Loader {
                    id: pageLoader
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
        id: loadingScreen
        z: 100
        anchors {
            top: titleBar.bottom
            left: parent.left
            leftMargin: chromeVisible ? sideBar.width : 0
            right: parent.right
            bottom: parent.bottom
        }
        loading: {
            switch (Globals.pageIndex) {
            case 0: return App.showManager.isLoading
            case 1: return App.playlist.isLoading
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
        anchors {
            top: titleBar.bottom
            left: parent.left
            leftMargin: chromeVisible ? sideBar.width : 0
            right: parent.right
            bottom: parent.bottom
        }
    }

    // Ctrl+K quick search overlay
    Rectangle {
        id: quickSearch
        anchors.fill: parent
        z: 200
        color: "#99000000"
        property bool open: false
        visible: opacity > 0.01
        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
        onOpenChanged: {
            opacity = open ? 1 : 0
            if (open) { qsField.text = ""; qsField.forceActiveFocus() }
        }

        MouseArea { anchors.fill: parent; onClicked: quickSearch.open = false }

        Rectangle {
            anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: parent.height * 0.18 }
            width: Math.min(560, parent.width - 80)
            height: 58
            radius: 14
            color: Theme.surface
            border.color: Theme.accent; border.width: 1
            MouseArea { anchors.fill: parent }   // swallow clicks so the box stays open
            Text {
                anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 18 }
                text: "⌕"; color: Theme.textMuted; font.pixelSize: 22
            }
            Text {
                anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 50 }
                visible: qsField.text.length === 0
                text: qsTr("Search anime, shows, movies...")
                color: Theme.textMuted
                font.family: "QTxiaotu"
                font.pixelSize: Globals.sp(20)
            }
            TextField {
                id: qsField
                anchors { fill: parent; leftMargin: 50; rightMargin: 16 }
                verticalAlignment: TextInput.AlignVCenter
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                color: Theme.textPrimary
                font.family: "QTxiaotu"
                font.pixelSize: Globals.sp(20)
                background: null
                selectByMouse: true
                onAccepted: {
                    if (text.trim().length === 0) return
                    Globals.lastSearch = text
                    App.explore(text, 1, false)
                    gotoPage(0)
                    quickSearch.open = false
                }
                Keys.onEscapePressed: quickSearch.open = false
            }
        }
    }

    Popup {
        id: notifier
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        property int notifierPadding: 20
        width: Math.min(520, Math.round(parent.width * 0.9))
        implicitHeight: column.implicitHeight + notifierPadding * 2
        anchors.centerIn: parent

        Overlay.modal: Rectangle { color: "#00000099" }

        background: Item {
            implicitWidth: 400
            implicitHeight: 220
            Rectangle {
                id: bgCard
                anchors.fill: parent
                radius: 14
                color: Theme.surface
                border.color: "#4E5BF233"
                border.width: 1
            }
            DropShadow {
                anchors.fill: bgCard
                source: bgCard
                verticalOffset: 12
                radius: 24
                samples: 32
                color: "#00000088"
                transparentBorder: true
            }
        }

        enter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 160
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "scale"
                    from: 0.94
                    to: 1.0
                    duration: 180
                    easing.type: Easing.OutBack
                }
            }
        }
        exit: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 140
                    easing.type: Easing.InCubic
                }
                NumberAnimation {
                    property: "scale"
                    from: 1.0
                    to: 0.96
                    duration: 140
                    easing.type: Easing.InCubic
                }
            }
        }

        contentItem: ColumnLayout {
            id: column
            spacing: 14
            width: parent ? parent.width - (notifier.notifierPadding * 2) : 400

            Text {
                id: headerText
                text: "Error"
                color: "#FCA5A5"
                font {
                    pixelSize: Globals.sp(24)
                    bold: true
                }
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(220, Math.max(80, implicitHeight))
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Text {
                    id: notifierMessage
                    text: "An error has occurred."
                    wrapMode: Text.Wrap
                    color: Theme.textPrimary
                    opacity: 0.9
                    font.pixelSize: Globals.sp(20)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                AppButton {
                    text: "View Logs"
                    backgroundDefaultColor: "#374151"
                    contentItemTextColor: Theme.textPrimary
                    cornerRadius: 10
                    fontSize: 20
                    onClicked: {
                        notifier.close()
                        gotoPage(5)
                    }
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    id: okButton
                    text: "OK"
                    cornerRadius: 10
                    fontSize: 20
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

        onOpened: okButton.forceActiveFocus()
        onClosed: {
            if (mpvPage.visible) mpvPage.forceActiveFocus()
            else focusCurrentPage()
        }
    }

    Image {
        id: debugOverlay
        anchors.fill: parent
        visible: false
        source: "qrc:/AoNami/resources/images/periodic-table.jpg"
    }

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
            let next = Globals.pageIndex + 1
            if (next === 1 && !App.showManager.currentShow.exists) next++
            gotoPage(next % pageCount)
        }
    }
    Shortcut {
        sequence: "Ctrl+Shift+Tab"
        onActivated: {
            let prev = Globals.pageIndex - 1
            if (prev === 1 && !App.showManager.currentShow.exists) prev--
            gotoPage(prev < 0 ? pageCount - 1 : prev)
        }
    }

    Shortcut { sequence: "Ctrl+W"; onActivated: root.close() }
    Shortcut { sequence: "Ctrl+K"; onActivated: quickSearch.open = !quickSearch.open }
    Shortcut { sequence: "1"; onActivated: gotoPage(0) }
    Shortcut { sequence: "2"; onActivated: gotoPage(1) }
    Shortcut { sequence: "3"; onActivated: gotoPage(2) }
    Shortcut { sequence: "4"; onActivated: gotoPage(3) }
    Shortcut { sequence: "5"; onActivated: gotoPage(4) }
    Shortcut { sequence: "6"; onActivated: gotoPage(5) }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: {
            if (Globals.pipMode) togglePip()
            if (Globals.maximised) toggleMaximised()
            if (Globals.fullscreen) toggleFullscreen()
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