import QtQuick.Controls 2.0
import QtQuick 2.15
import QtQuick 2.4

TextField {
    id: textField
    property color checkedColor: "#D5DBDB"
    property color textColor: "white"
    property color accentColor: "#4E5BF2"
    property int fontSize: 20
    readonly property int scaledFontSize: fontSize * (root.maximised ? fontScaleFactor : 1)
    property int fontScaleFactor: 2

    signal doubleClicked(var event)

    font.family: "QTxiaotu"

    font.pixelSize: scaledFontSize
    font.weight: Font.Thin
    color: textField.textColor

    antialiasing: true
    hoverEnabled: true

    background: Rectangle {
        radius: parent.height/2
        color: textField.enabled ? "transparent" : "#F4F6F6"
        border.color: textField.activeFocus ? textField.accentColor : (textField.hovered ? Qt.rgba(255, 255, 255, 0.2) : textField.checkedColor)
        border.width: 2
        opacity: textField.enabled ? 1 : 0.7
    }
    leftPadding: 12
    rightPadding: 12

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        acceptedButtons: Qt.NoButton
    }

    onDoubleClicked: selectAll()
}



