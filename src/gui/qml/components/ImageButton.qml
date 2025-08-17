import QtQuick 2.15
import QtQuick.Controls 2.15

Image {
    id: imageButton

    property string hoverImage: "" // Source for the image on hover
    property string image: "" // Source for the image on display
    property alias cursorShape: mouseArea.cursorShape
    property bool selected: false
    property bool enabled: true
    property bool hovered: false
    property string tooltipText: ""

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
    }
    onHoverImageChanged: {
        if (mouseArea.containsMouse && hoverImage !== "") {
            imageButton.source = hoverImage;
        }
    }

    fillMode: Image.PreserveAspectFit
    transformOrigin: Item.Center
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

    Rectangle {
        id: highlight
        anchors.fill: parent
        radius: Math.min(width, height) * 0.2
        color: selected ? "#2a324833" : (hovered ? "#1b213366" : "transparent")
        border.color: selected ? "#4E5BF2" : "transparent"
        border.width: selected ? 1 : 0
        z: -1
    }

    Rectangle {
        id: accentBar
        width: 3
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            topMargin: parent.height * 0.15
            bottomMargin: parent.height * 0.15
        }
        radius: width
        color: "#4E5BF2"
        visible: selected
        opacity: 0.9
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: if (imageButton.enabled) imageButton.clicked()
        cursorShape: Qt.PointingHandCursor

        onPressed: imageButton.scale = 0.95
        onReleased: imageButton.scale = hovered ? 1.08 : 1.0

        onContainsMouseChanged: {
            hovered = containsMouse
            if (imageButton.hoverImage !== "") {
                imageButton.source = containsMouse ? imageButton.hoverImage : imageButton.image; // Assuming the source hasn't dynamically changed while hovering
            }
            imageButton.scale = containsMouse ? 1.08 : 1.0
        }
    }

    ToolTip.visible: false
}









