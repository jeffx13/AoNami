import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: button
    property string image: ""
    property string hoverImage: ""
    property bool selected: false
    hoverEnabled: true

    background: Image {
        mipmap: true
        source: button.hovered ? hoverImage : image
        sourceSize.width: parent.width-border.width*2
        sourceSize.height: parent.height-border.width*2
    }
    focusPolicy: Qt.NoFocus

    Rectangle {
        id: border
        visible: selected
        anchors.fill: parent
        color: "transparent" // Set the initial color to transparent
        border.color: "white" // Set the border color
        border.width: 1
    }



    // topInset and bottomInset is available after Qt5.12
    Component.onCompleted: {
        if (button.topInset !== undefined)
        {
            button.topInset = 0;
            button.bottomInset = 0;
        }
    }
}
