import QtQuick
import ".."

Rectangle {
    property alias source: art.source

    radius: 6
    clip: true
    color: Theme.surfaceDeep

    Image {
        id: art
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        sourceSize: Qt.size(width * 2, height * 2)
    }
}
