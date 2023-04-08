import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: button
    property string image: ""
    property string hoverImage: ""

    hoverEnabled: true

    background: Image {
        mipmap: true
        source: button.hovered ? hoverImage : image
        sourceSize.width: parent.width
        sourceSize.height: parent.height
    }
    focusPolicy: Qt.NoFocus

    // topInset and bottomInset is available after Qt5.12
    Component.onCompleted: {
        if (button.topInset !== undefined)
        {
            button.topInset = 0;
            button.bottomInset = 0;
        }
    }
}
