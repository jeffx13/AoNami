import QtQuick
import ".."

Item {
    id: rotatingText
    property string text: ""
    property color color: "#cfd5e6"
    property int fontSize: 20
    property int spacing: 30
    property real marqueeSpeed: 80
    property bool pauseOnHover: true
    property bool play: true
    property bool isHovered: false
    property bool isOverflow: primaryText.paintedWidth > rotatingText.width
    property int horizontalAlignment: Text.AlignHCenter

    clip: true
    implicitHeight: Math.max(staticText.implicitHeight, primaryText.implicitHeight)
    visible: text.length > 0

    function updatePaused() {
        if (!rotatingText.pauseOnHover) {
            if (scrollAnim.paused) scrollAnim.paused = false
            return
        }
        if (!scrollAnim.running) {
            return
        }
        if (scrollAnim.paused !== rotatingText.isHovered) {
            scrollAnim.paused = rotatingText.isHovered
        }
    }

    onWidthChanged: {
        if (!scrollAnim.paused) {
            primaryText.x = 0
            if (scrollAnim.running) scrollAnim.restart()
        }
    }

    // Static centered text when it fits the container
    Text {
        id: staticText
        text: rotatingText.text
        font.pixelSize: Globals.sp(rotatingText.fontSize)
        color: rotatingText.color
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
        horizontalAlignment: rotatingText.horizontalAlignment
        anchors.verticalCenter: parent.verticalCenter
        visible: !rotatingText.isOverflow
    }

    Text {
        id: primaryText
        text: rotatingText.text
        font.pixelSize: Globals.sp(rotatingText.fontSize)
        color: rotatingText.color
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
        anchors.verticalCenter: parent.verticalCenter
        x: 0
        visible: rotatingText.isOverflow
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
        x: primaryText.x + primaryText.paintedWidth + rotatingText.spacing
        visible: rotatingText.isOverflow
    }

    NumberAnimation {
        id: scrollAnim
        target: primaryText
        property: "x"
        from: 0
        to: -(primaryText.paintedWidth + rotatingText.spacing)
        duration: ((primaryText.paintedWidth + rotatingText.spacing) / rotatingText.marqueeSpeed) * 1000
        easing.type: Easing.Linear
        loops: Animation.Infinite
        running: rotatingText.play && rotatingText.isOverflow
        onRunningChanged: rotatingText.updatePaused()
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onEntered: { rotatingText.isHovered = true; rotatingText.updatePaused() }
        onExited: { rotatingText.isHovered = false; rotatingText.updatePaused() }
    }
}


