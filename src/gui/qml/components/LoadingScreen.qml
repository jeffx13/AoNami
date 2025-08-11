import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
Item {
    Rectangle {
        anchors.fill:parent
        color:"black"
        opacity:0.3
        z:0
    }

    id: overlay
    anchors.fill:parent
    visible: loading
    signal cancelled()
    signal timedOut()
    property bool cancellable: true
    property bool loading: false
    property bool timeoutEnabled: false
    property int timeoutInterval: 5000



    onLoadingChanged: {
        // if (!timeoutEnabled) return
        if (loading) {
            // loadingTimer.start()
            loadingText.text = "Loading..."
        } else{
            // loadingTimer.stop()
            // loadingText.text = "Loading..."
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        z:1
        width: 150
        height: width * 2
        AnimatedImage {
            id: loadingAnimation
            source: "qrc:/resources/gifs/loading-totoro.gif"
            // width: 150
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 8
            visible: overlay.loading
            playing: overlay.loading
            Layout.alignment: Qt.AlignHCenter
        }
        Text {
            id: loadingText
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 1
            Layout.fillHeight: true
            // Layout.fillWidth: true
            text: "Loading..."
            color: "white"
            font.pixelSize: loadingAnimation.height * 0.1
            visible: overlay.loading
        }
        CustomButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 1
            Layout.fillHeight: true
            // Layout.fillWidth: true

            text: "Cancel"
            visible: overlay.cancellable
            onClicked: if (overlay.cancellable) {
                           loadingText.text = "Cancelling"
                           overlay.cancelled();
                       }
        }
    }





    Timer {
        id: loadingTimer
        interval: overlay.timeoutInterval
        running: false
        repeat: false

        onTriggered: if (cancellable) {
                         loadingText.text = "Cancelling"
                         overlay.cancelled();
                     }
    }
}

