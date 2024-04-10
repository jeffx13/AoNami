import QtQuick 2.15
import QtQuick.Controls 2.15


Image {
    id:imageButton

    property string hoverSource: "" // Source for the image on hover
    property alias cursorShape: mouseArea.cursorShape
    property bool selected
    property bool enabled: true
    signal clicked()

    fillMode: Image.PreserveAspectFit

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: if (imageButton.enabled) imageButton.clicked()
        cursorShape: Qt.PointingHandCursor
        //TODO
        // onContainsMouseChanged: {
        //     if (hoverSource !== "") {
        //         image.source = containsMouse ? hoverSource : source; // Assuming the source hasn't dynamically changed while hovering
        //     }
        // }
    }


}









