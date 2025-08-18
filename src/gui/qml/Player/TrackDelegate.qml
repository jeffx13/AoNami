import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: control
    property bool isCurrentIndex: false
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    Layout.alignment: Qt.AlignVCenter
    padding: 12

    contentItem: RowLayout {
        spacing: 10
        anchors.fill: parent
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 8
            height: 8
            radius: 4
            color: control.isCurrentIndex ? "#17a81a" : (control.hovered ? "#5a5f73" : "#3a3f4d")
            border.color: control.isCurrentIndex ? "#21be2b" : "#2a2f3a"
            border.width: 1
        }
        Text {
            id: label
            Layout.fillWidth: true
            verticalAlignment: Text.AlignVCenter
            text: control.text
            font.pixelSize: 20 * root.fontSizeMultiplier
            elide: Text.ElideRight
            color: control.isCurrentIndex ? "#E6FFE8" : (control.hovered ? "#FFFFFF" : "#E0E0E0")
            opacity: enabled ? 1.0 : 0.3
        }
    }

    background: Rectangle {
        implicitHeight: 44
        color: control.isCurrentIndex ? "#1c2a22" : (control.hovered ? "#151924" : "#0f1115")
        border.color: control.isCurrentIndex ? "#21be2b" : (control.hovered ? "#2a2f3a" : "#1a1f2a")
        border.width: 1
        radius: 8
    }

    Behavior on scale { NumberAnimation { duration: 100 } }
    scale: control.down ? 0.98 : 1.0
}
