import QtQuick
import QtQuick.Controls
import ".."

// A raster icon that behaves like a button; the caller sets `source` directly.
Image {
    id: btn

    signal clicked()

    fillMode: Image.PreserveAspectFit

    scale: area.pressed ? 0.92 : (area.containsMouse ? 1.06 : 1.0)
    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: if (btn.enabled) btn.clicked()
    }
}
