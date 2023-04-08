import QtQuick 2.15
import QtQuick.Controls 2.15
Rectangle {
    id: loadingScreen
//    width: parent.width
//    height: parent.height
    color: "#80000000"
    visible: busyIndicator.running
    z:100
    function startLoading(){
        busyIndicator.running = true
        busyIndicator.visible = true
        parent.enabled = false
    }
    function stopLoading(){
        busyIndicator.running = false
        busyIndicator.visible = false
        parent.enabled = true
    }

    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        running: false
        visible: false
        z:100
        height: 100
        width: 100
    }
    Timer {
        id: loadingTimer
        interval: 5000 // set the timeout interval to 5 seconds
        running: false
        repeat: false // don't repeat the timer

        onTriggered: {
            loadingScreen.visible = false
            errorPopup.open()
            // perform any additional actions you want when the timeout is reached
        }
    }
}


