import QtQuick
import QtQuick.Layouts
import ".."

Item {
    id: overlay
    visible: loading

    signal cancelled()
    property bool cancellable: false   // opt in, so a call site without onCancelled can't show a dead button
    property bool loading: false

    // Scrim and label flip together: a dark scrim on a light theme would hide textPrimary.
    Rectangle {
        anchors.fill: parent
        color: Theme.isLight ? "#FFFFFF" : "#000000"
        opacity: Theme.isLight ? 0.55 : 0.3
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
            color: Theme.textPrimary
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