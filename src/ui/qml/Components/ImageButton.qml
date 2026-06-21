import QtQuick
import QtQuick.Controls
import ".."

Image {
    id: btn

    property string image: ""
    property string hoverImage: ""
    property bool   selected: false
    property alias  cursorShape: area.cursorShape

    signal clicked()

    source: (area.containsMouse && hoverImage !== "") ? hoverImage : image
    fillMode: Image.PreserveAspectFit

    scale: area.pressed ? 0.92 : (area.containsMouse ? 1.06 : 1.0)
    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

    Rectangle {
        anchors.fill: parent; z: -1
        radius: Math.min(width, height) * 0.2
        color: btn.selected ? "#224E5BF2" : "transparent"
        border.color: btn.selected ? Theme.accent : "transparent"
        border.width: btn.selected ? 1 : 0
    }

    Rectangle {
        visible: btn.selected
        width: 3; radius: width
        color: Theme.accent; opacity: 0.9
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom; topMargin: parent.height * 0.15; bottomMargin: parent.height * 0.15 }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: if (btn.enabled) btn.clicked()
    }
}