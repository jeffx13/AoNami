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
    onClosing: App.play.saveProgress()

    color: Theme.background
    Material.theme: Material.Dark
    Material.primary: Theme.background
    Material.accent: Theme.accent
    Material.foreground: Theme.textPrimary

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

        if (App.play.playPlaylist(0)) {
            Globals.pageIndex = 3
            history = [3]
        } else if (!App.explorer.isLoading && App.searchResultModel.count === 0) {
            App.explore("", 1, true)
        }

        deferredStartupTimer.start()
    }

    property var history: [0]
    property int historyIndex: 0

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
        height: chromeVisible ? 38 : 0
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Theme.background
            }
            GradientStop {
                position: 0.5
                color: Theme.surface
            }
            GradientStop {
                position: 1.0
                color: Theme.surfaceAlt
            }
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

        Text {
            anchors.centerIn: parent
            text: "AoNami"
            color: "#4B5563"
            font {
                pixelSize: 13
                letterSpacing: 1.5
            }
            opacity: 0.7
        }

        Row {
            id: trafficLights
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 12
            }
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
                    width: 14
                    height: 14
                    radius: 7
                    HoverHandler { id: dotHover }
                    color: dotHover.hovered    ? modelData.dotColor
                         : groupHandler.hovered ? modelData.groupColor
                         : "#3B4252"
                    border.color: dotHover.hovered ? modelData.borderColor : "#ffffff15"
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        visible: groupHandler.hovered
                        text: index === 1 ? (Globals.maximised ? "\u274F" : "\u25FB")
                                          : (modelData.icon ?? "")
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
        width: chromeVisible ? 56 : 0
        anchors {
            left: parent.left
            top: titleBar.bottom
            bottom: parent.bottom
        }

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Theme.background
            }
            GradientStop {
                position: 0.5
                color: "#0E162B"
            }
            GradientStop {
                position: 1.0
                color: "#101934"
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 5

            Repeater {
                model: [
                    { page: 0, icon: "search",   selectedIcon: "search_selected" },
                    { page: 1, icon: "details",  selectedIcon: "details_selected", needsShow: true },
                    { page: 2, icon: "library",  selectedIcon: "library_selected" },
                    { page: 3, icon: "tv",       selectedIcon: "tv_selected" },
                    { page: 4, icon: "download", selectedIcon: "download_selected" },
                    { page: 5, icon: "log",      selectedIcon: "log_selected" },
                    { page: 6, icon: "settings", selectedIcon: "settings_selected" }
                ]
                delegate: ImageButton {
                    required property var modelData
                    property bool isSelected: Globals.pageIndex === modelData.page
                    enabled: modelData.needsShow ? App.showManager.currentShow.exists : true
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    image: isSelected
                        ? "qrc:/AoNami/resources/images/" + modelData.selectedIcon + ".png"
                        : "qrc:/AoNami/resources/images/" + modelData.icon + ".png"
                    selected: isSelected
                    Layout.preferredWidth: sideBar.width
                    Layout.preferredHeight: sideBar.width
                    onClicked: gotoPage(modelData.page)
                }
            }

            Item { Layout.fillHeight: true }
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
            left: sideBar.right
            right: parent.right
            bottom: parent.bottom
        }
        opacity: Globals.pageIndex === 3 ? 0 : 1
        visible: opacity > 0.001
        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

        // Cache the page as one texture during the cross-fade so it isn't re-rendered every frame.
        layer.enabled: opacity > 0.001 && opacity < 0.999

        Rectangle { anchors.fill: parent; color: Theme.background }

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
            left: sideBar.right
            right: parent.right
            bottom: parent.bottom
        }
        loading: {
            switch (Globals.pageIndex) {
            case 0: return App.showManager.isLoading
            case 1: return App.play.isLoading
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
                if (App.play.isLoading) App.play.cancel()
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
            left: sideBar.right
            right: parent.right
            bottom: parent.bottom
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