import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: overlay
    anchors.fill: parent
    z: parent.z + 1
    visible: loading

    signal cancelled()
    property bool cancellable: true
    property bool loading: false
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.3
    }

    onLoadingChanged: {
        if (loading)
            loadingText.text = "Loading..."
    }

    ColumnLayout {
        anchors {
            verticalCenter: parent.verticalCenter
            horizontalCenter: parent.horizontalCenter
        }
        width: parent.width / 6
        height: parent.height / 2

        spacing: 5
        AnimatedImage {
            id: loadingAnimation
            source: "qrc:/AoNami/resources/gifs/loading-totoro.gif"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 96
            fillMode: Image.PreserveAspectFit
            visible: overlay.loading
            playing: overlay.loading
            Layout.alignment: Qt.AlignHCenter
        }
        Text {
            id: loadingText
            Layout.alignment: Qt.AlignHCenter
            text: "Loading..."
            color: "white"
            font.pixelSize: Globals.sp(16)
            visible: overlay.loading
        }
        AppButton {
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


