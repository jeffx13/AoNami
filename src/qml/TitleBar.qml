import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    color: "#E6404040"
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: 35
    property QtObject container
    focus: false
    MouseArea {
        property var clickPos
        anchors.fill: parent
        onPressed: (mouse)=>{
                       clickPos  = Qt.point(mouse.x,mouse.y)
                   }
        onPositionChanged: (mouse)=> {
                               container.x = mousePosition.cursorPos().x - clickPos.x
                               container.y = mousePosition.cursorPos().y - clickPos.y
                           }
        onDoubleClicked: {
            setMaximised(!window.maximised)
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
        onClicked: window.close()
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
        onClicked: setMaximised(!window.maximised)
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
            window.showMinimized()
        }
        focusPolicy: Qt.NoFocus
    }

    Button {
        id:testButton
        width: parent.height*1.5
        height: parent.height
        anchors.top: parent.top
        anchors.left: parent.left
        background: Rectangle { color: "#ffbf39"; anchors.fill: parent }
        onClicked: removeMpv()
        text: "mpv"
        focusPolicy: Qt.NoFocus

        //        id: navBarButton
        //        width: parent.height*1.5
        //        height: parent.height
        //        anchors.top: parent.top
        //        anchors.left: parent.left
        //        background: Rectangle { color: "#ffbf39"; anchors.fill: parent }

        //        onClicked: {
        //            if(sideMenuDrawer.opened){
        //                sideMenuDrawer.close()
        //                delay(500,function() {
        //                    //                            if(previousMpvState===1)mpv.play()
        //                })
        //            }else{
        //                //                        previousMpvState = mpv.state
        //                //                        mpv.pause()
        //                //                        sideMenuDrawer.open()
        //            }
        //            //                    videoSettingAnimation.start()
        //        }
    }
    Button {
        width: parent.height*3
        height: parent.height
        anchors.top: parent.top
        anchors.left: testButton.right
        background: Rectangle { color: "#fa564d"; anchors.fill: parent }
        onClicked: folderDialog.open()
        text: "folder"
    }



}

