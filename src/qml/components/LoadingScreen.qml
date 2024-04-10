import QtQuick 2.15
import QtQuick.Controls 2.15
Popup {
    id: overlay
    dim: true
    anchors.centerIn: parent
    visible: loading
    closePolicy: Popup.NoAutoClose
    signal cancelled()
    signal timedOut()
    property bool cancellable: true
    property bool loading: false
    property bool timeoutEnabled: true
    property int timeoutInterval: 5000
    background: Rectangle {
        color: "black"
    }
    onLoadingChanged: {
        if (!timeoutEnabled) return
        if (loading) {
            loadingTimer.start()
        } else{
            loadingTimer.stop()
        }
    }

    AnimatedImage {
        id: loadingAnimation
        anchors.centerIn: parent
        source: "qrc:/resources/gifs/loading-totoro.gif"
        width: 150
        height: width * 1.5
        visible: loading
        playing: loading
    }
    CustomButton {
        anchors.horizontalCenter: loadingAnimation.horizontalCenter
        anchors.top: loadingAnimation.bottom
        width: loadingAnimation.width * 0.5
        height: loadingAnimation.height * 0.1

        text: "Cancel"
        visible: cancellable
        onClicked: if (cancellable) { cancelled(); overlay.close() }
    }


    Timer {
        id: loadingTimer
        interval: timeoutInterval
        running: false
        repeat: false

        onTriggered: {
            cancelled()
            timedOut()
            overlay.close()
        }
    }
}

