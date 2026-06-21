import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: overlay
    anchors.fill: parent
    visible: loading

    signal cancelled()
    property bool cancellable: true
    property bool loading: false

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.3
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width / 6
        spacing: 8

        AnimatedImage {
            source: "qrc:/AoNami/resources/gifs/loading-totoro.gif"
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            fillMode: Image.PreserveAspectFit
            playing: overlay.loading
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            id: loadingText
            Layout.alignment: Qt.AlignHCenter
            text: "Loading..."
            color: "white"
            font.pixelSize: Globals.sp(20)
        }

        AppButton {
            Layout.alignment: Qt.AlignHCenter
            text: "Cancel"
            visible: overlay.cancellable
            onClicked: {
                loadingText.text = "Cancelling..."
                overlay.cancelled()
            }
        }
    }

    onLoadingChanged: {
        if (loading) loadingText.text = "Loading..."
    }
}