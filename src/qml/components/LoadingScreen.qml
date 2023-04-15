import QtQuick 2.15
import QtQuick.Controls 2.15
Rectangle {
    id: loadingScreen
    property bool loading:false
    color: "#80000000"
    visible: busyIndicator.running
    z:parent.z+1

    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        running: loading
        visible: loading
        z:loadingScreen.z
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

