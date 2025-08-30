pragma Singleton
import QtQml

QtObject {
    id: globals

    // Window state
    property bool maximised: false
    property bool fullscreen: false
    property bool pipMode: false

    // Dimensions
    property real appWidth: root ? root.width : 0
    property real appHeight: root ? root.height : 0

    // Navigation and history
    property int pageIndex: 0

    // Misc shared state
    property var root
    property var mpv
    property real libraryLastContentY: 0
    property real explorerLastContentY: 0
    property string lastSearch: ""
    property real imageAspectRatio: 319/225

    // Derived
    property real fontSizeMultiplier: maximised ? 1.6 : 1
    function sp(n) {
        return Math.round(n * fontSizeMultiplier)
    }

    function gotoPage(index) {
        root.gotoPage(index)
    }
    function togglePip() {
        root.togglePip()
    }
    function toggleFullscreen() {
        root.toggleFullscreen()
    }
    function toggleMaximised() {
        root.toggleMaximised()
    }

}


