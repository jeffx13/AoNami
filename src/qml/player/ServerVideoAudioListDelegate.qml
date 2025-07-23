import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Button {
    id: control

    // width: serverListView.width
    // height: serverListView.height / 5
    // onClicked: {}
    property bool isCurrentIndex: false

    Layout.alignment: Qt.AlignVCenter

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: control.down ? "red" : "#fcfcfc"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        opacity: enabled ? 1 : 0.3
        border.color: control.down ? "#17a81a" : "#21be2b"
        border.width: 1
        color: isCurrentIndex ? "purple" : "black"
        radius: 2
    }
}
