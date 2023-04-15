import QtQuick
import QtQuick.Window 2.2
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MpvPlayer 1.0

import "./explorer"
import "./info"
import "./player"
import "./components"

Window {
    id: window
    width: 1080
    height: 720
    visible: true
    color: "black"
    flags: Qt.Window | Qt.FramelessWindowHint |Qt.WindowMinimizeButtonHint
    title: ""//qsTr("kyokou")
    property bool maximised: window.visibility === Window.FullScreen
    property var mpv : mpvPage.mpv
    //    onActiveFocusItemChanged: {
    //        console.log(activeFocusItem)
    //    console.log(activeFocusItem.dumpItemTree())}

    function setMaximised(shouldFullscreen){
        if(shouldFullscreen){
            window.visibility = Window.FullScreen;
        }else{
            window.visibility = Window.AutomaticVisibility;
        }
    }

    function setPlayerFullscreen(shouldFullscreen){
        setMaximised(shouldFullscreen)
        playerIsFullScreen = shouldFullscreen
    }

    property bool playerIsFullScreen:false






    Rectangle{
        id:viewRect
        anchors.fill: parent
        TitleBar{
            id:titleBar
            focus: false
        }

        SideBar{
            id:sideBar
            anchors{
                left: parent.left
                top:titleBar.bottom
                bottom:parent.bottom
            }
        }

        StackView{
            visible: !mpvPage.visible
            id:stackView
            anchors{
                top: playerIsFullScreen ? parent.top: titleBar.bottom
                left: playerIsFullScreen ? parent.left : sideBar.right
                right: parent.right
                bottom: parent.bottom
            }
            initialItem: "explorer/SearchPage.qml"

            background: Rectangle{
                color: "black"
            }
        }
        MpvPage{
            id:mpvPage
            visible: false
            anchors.fill: stackView

        }
    }
    MouseArea{
        anchors.fill: parent
        acceptedButtons: Qt.ForwardButton | Qt.BackButton
        onClicked: (mouse)=>{
                       if(playerIsFullScreen)return;
                       if(mouse.button === Qt.BackButton){
                           let nextPage = sideBar.currentPage+1
                           sideBar.gotoPage(nextPage === Object.keys(sideBar.pages).length ? 0 : nextPage)
                       }else{

                           let prevPage = sideBar.currentPage-1
                           sideBar.gotoPage(prevPage < 0 ? Object.keys(sideBar.pages).length-1 : prevPage)
                       }
                   }
    }


    Dialog {
        id: errorPopup
        modal: true
        width: 400
        height: 150
        contentItem: Rectangle {
            color: "#f2f2f2"
            border.color: "#c2c2c2"
            border.width: 1
            radius: 10
            anchors.centerIn: parent
            Text {
                text: "Error"
                font.pointSize: 16
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 20
            }
            Text {
                id: errorMessage
                text: "An error has occurred."
                font.pointSize: 14
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
            Button {
                text: "OK"
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: errorPopup.close()
            }
        }
    }





}
