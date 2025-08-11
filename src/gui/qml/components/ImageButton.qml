import QtQuick 2.15
import QtQuick.Controls 2.15


Image {
    id:imageButton

    property string hoverImage: "" // Source for the image on hover
    property string image: "" // Source for the image on display
    property alias cursorShape: mouseArea.cursorShape
    property bool selected
    property bool enabled: true

    signal clicked()
    onImageChanged: {
        if (image !== "") {
            imageButton.source = image;
        }
    }
    Component.onCompleted: {
        if (image !== "") {
            imageButton.source = image;
        }
        mouseArea.hoverEnabled = imageButton.hoverImage !== ""
    }
    onHoverImageChanged: {
        mouseArea.hoverEnabled = hoverImage !== ""
        if (mouseArea.hoverEnabled && mouseArea.containsMouse) {
            imageButton.source = hoverImage;
        }
    }

    fillMode: Image.PreserveAspectFit

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: if (imageButton.enabled) imageButton.clicked()
        cursorShape: Qt.PointingHandCursor

        onContainsMouseChanged: {
            if (imageButton.hoverImage !== "") {
                imageButton.source = containsMouse ? imageButton.hoverImage : imageButton.image; // Assuming the source hasn't dynamically changed while hovering
            }
        }
    }


}









