import QtQuick
import ".."

Item {
    id: marqueeText
    property string text: ""
    property color color: Theme.textSecondary
    property int fontSize: 20
    property int spacing: 30
    property real marqueeSpeed: 80
    readonly property bool isOverflow: primaryText.paintedWidth > marqueeText.width
    property int horizontalAlignment: Text.AlignHCenter

    clip: true
    implicitHeight: Math.max(staticText.implicitHeight, primaryText.implicitHeight)
    visible: text.length > 0

    // setPaused() warns when the animation is not running, so the guard has to stay.
    function updatePaused(hovered) {
        if (scrollAnim.running && scrollAnim.paused !== hovered) scrollAnim.paused = hovered
    }

    onWidthChanged: {
        if (!scrollAnim.paused) {
            primaryText.x = 0
            if (scrollAnim.running) scrollAnim.restart()
        }
    }

    Text {
        id: staticText
        // Needs full width or horizontalAlignment has nothing to align within.
        width: marqueeText.width
        text: marqueeText.text
        font.pixelSize: Globals.sp(marqueeText.fontSize)
        color: marqueeText.color
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
        horizontalAlignment: marqueeText.horizontalAlignment
        anchors.verticalCenter: parent.verticalCenter
        visible: !marqueeText.isOverflow
    }

    Text {
        id: primaryText
        text: marqueeText.text
        font.pixelSize: Globals.sp(marqueeText.fontSize)
        color: marqueeText.color
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
        anchors.verticalCenter: parent.verticalCenter
        x: 0
        visible: marqueeText.isOverflow
        onPaintedWidthChanged: if (scrollAnim.running && !scrollAnim.paused) scrollAnim.restart()
        onTextChanged: {
            x = 0
            if (scrollAnim.running) scrollAnim.restart()
        }
    }

    Text {
        text: primaryText.text
        font.pixelSize: primaryText.font.pixelSize
        color: primaryText.color
        wrapMode: Text.NoWrap
        anchors.verticalCenter: parent.verticalCenter
        x: primaryText.x + primaryText.paintedWidth + marqueeText.spacing
        visible: marqueeText.isOverflow
    }

    NumberAnimation {
        id: scrollAnim
        target: primaryText
        property: "x"
        from: 0
        to: -(primaryText.paintedWidth + marqueeText.spacing)
        duration: ((primaryText.paintedWidth + marqueeText.spacing) / marqueeText.marqueeSpeed) * 1000
        easing.type: Easing.Linear
        loops: Animation.Infinite
        running: marqueeText.isOverflow
        onRunningChanged: marqueeText.updatePaused(hover.hovered)
    }

    HoverHandler {
        id: hover
        onHoveredChanged: marqueeText.updatePaused(hover.hovered)
    }
}


