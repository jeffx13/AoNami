pragma Singleton
import QtQml

QtObject {
    id: globals

    property bool maximised: false
    property bool fullscreen: false
    property bool pipMode: false

    property real appWidth:  root ? root.width  : 0
    property real appHeight: root ? root.height : 0

    property int pageIndex: 0

    property var root: null
    property var mpv: null

    property real libraryLastContentY: 0
    property real explorerLastContentY: 0
    property string lastSearch: ""
    property real imageAspectRatio: 319 / 225

    readonly property int defaultWidth: 1080
    readonly property int defaultHeight: 720

    readonly property var libraryTypes: ["Watching", "Planned", "Paused", "Dropped", "Completed"]

    property real uiScale: 1.0   // user setting, fed from main.qml

    // Text stays a consistent size regardless of window size; the UI Scale setting
    // is the single knob for making everything bigger/smaller.
    function sp(n) {
        return Math.round(n * uiScale)
    }

    function gotoPage(index)    { if (root) root.gotoPage(index) }
    function togglePip()        { if (root) root.togglePip() }
    function toggleFullscreen() { if (root) root.toggleFullscreen() }
    function toggleMaximised()  { if (root) root.toggleMaximised() }
}
