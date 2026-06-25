import QtQuick
import Qt5Compat.GraphicalEffects
import ".."

Item {
    id: root
    property string name: ""
    property color color: Theme.textSecondary
    property int size: 20
    implicitWidth: size
    implicitHeight: size

    Image {
        id: img
        anchors.fill: parent
        source: root.name ? "qrc:/AoNami/resources/icons/" + root.name + ".svg" : ""
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
        visible: false
    }
    ColorOverlay {
        anchors.fill: img
        source: img
        color: root.color
    }
}
