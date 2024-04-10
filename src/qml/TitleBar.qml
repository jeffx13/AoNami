import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Shapes 1.15
Rectangle {
    color: "#9BC7EE"
    gradient: Gradient {
            GradientStop { position: 0.0; color: "#F3B1BF" }
            GradientStop { position: 0.5; color: "#96C8ED" }
            GradientStop { position: 1.0; color: "#B7BADB" }
        }

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: 35

    MouseArea {
        property var clickPos
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse)=>{
                       clickPos  = Qt.point(mouse.x,mouse.y)
                   }
        onPositionChanged: (mouse)=> {
                               if (!root.maximised && clickPos !== null){
                                   root.x = app.cursor.pos().x - clickPos.x
                                   root.y = app.cursor.pos().y - clickPos.y
                               }
                           }
        onDoubleClicked: {
            root.maximised = !root.maximised
            clickPos = null
        }

    }

    Button  {
        id: closeButton
        width: 14
        height: 14
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 8
        background: Rectangle { color:  "#fa564d"; radius: 7; anchors.fill: parent }
        onClicked: {app.updateTimeStamp(); root.close()}
        focusPolicy: Qt.NoFocus
    }

    Button {
        id: maxButton
        width: 14
        height: 14
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: closeButton.left
        anchors.rightMargin: 6
        background: Rectangle { color: "#ffbf39"; radius: 7; anchors.fill: parent }
        onClicked: root.maximised = !root.maximised
        focusPolicy: Qt.NoFocus

    }

    Button {
        id: minButton
        width: 14
        height: 14
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: maxButton.left
        anchors.rightMargin: 6
        background: Rectangle { color: "#53cb43"; radius: 7; anchors.fill: parent }
        onClicked: {
            root.showMinimized()
        }
        focusPolicy: Qt.NoFocus
    }




}
