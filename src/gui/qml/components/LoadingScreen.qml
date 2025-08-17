import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
Item {
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.3
        z: 0
    }

    id: overlay
    anchors.fill: parent
    visible: loading
    signal cancelled()
    signal timedOut()
    property bool cancellable: true
    property bool loading: false
    property bool timeoutEnabled: false
    property int timeoutInterval: 5000

    onLoadingChanged: {
        if (loading) {
            loadingText.text = "Loading..."
        } else{
            // no-op
        }
    }

    Rectangle {
        id: card
        width: Math.min(parent.width * 0.4, 360)
        height: 220
        radius: 16
        color: "#151923"
        opacity: 0.96
        border.color: "#2B2F44"
        border.width: 1
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            AnimatedImage {
                id: loadingAnimation
                source: "qrc:/resources/gifs/loading-totoro.gif"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 96
                visible: overlay.loading
                playing: overlay.loading
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                id: loadingText
                Layout.alignment: Qt.AlignHCenter
                text: "Loading..."
                color: "white"
                font.pixelSize: 16
                visible: overlay.loading
            }
            CustomButton {
                Layout.alignment: Qt.AlignHCenter
                text: "Cancel"
                visible: overlay.cancellable
                onClicked: if (overlay.cancellable) {
                    loadingText.text = "Cancelling"
                    overlay.cancelled();
                }
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

