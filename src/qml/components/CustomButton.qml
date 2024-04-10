import QtQuick.Controls
import QtQuick 2.15

Button {
    id: button
    property color backgroundDefaultColor: "#4E5BF2"
    property color backgroundPressedColor: Qt.darker(backgroundDefaultColor, 1.2)
    property color contentItemTextColor: "white"
    property int fontSize: 20
    readonly property int scaledFontSize: fontSize * (root.maximised ? fontScaleFactor : 1)
    property int fontScaleFactor: 2
    property alias radius: backRect.radius


    contentItem: Text {
        text: button.text
        color: button.contentItemTextColor
        font.weight: Font.Thin
        font.pixelSize: scaledFontSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        wrapMode: Text.Wrap
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        onPressed: (mouse) => {
                       backRect.color = button.backgroundPressedColor
                   }
        onReleased: {
            backRect.color = button.backgroundDefaultColor
        }
        cursorShape: Qt.PointingHandCursor

    }
    focusPolicy: Qt.NoFocus
    background: Rectangle {
        id: backRect

        color: button.backgroundDefaultColor
        radius: 3
    }
}

