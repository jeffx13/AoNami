import QtQuick
import ".."


Item {
    id: showItem
    property alias showTitle: showTitleText.text
    property alias showCover: showImage.source
    property real aspectRatio: 319/225 // default
    property alias image: imageClip
    signal imageClicked(var mouse)
    signal imageLoaded(real sourceAspectRatio)


    Rectangle {
        id: card
        anchors.fill: parent
        color: "transparent"
        radius: 12
        border.color: "#2B2F44"
        border.width: 1
        opacity: 1

        states: [
            State { name: "hover"; when: mouseArea.containsMouse; PropertyChanges { target: card; border.color: "#4E5BF2" } }
        ]

        transitions: [
            Transition { NumberAnimation { properties: "opacity"; duration: 150 } }
        ]

        Rectangle {
            id: imageClip
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 2
            }
            height: width * aspectRatio
            radius: 10
            color: "transparent"
            clip: true

            Image {
                id: showImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                onStatusChanged: {
                    if (showImage.status === Image.Error) source = "qrc:/AoNami/resources/images/error_image.png"
                    if (showImage.status === Image.Ready && showImage.sourceSize.width > 0 && showImage.sourceSize.height > 0) {
                        showItem.imageLoaded(showImage.sourceSize.height / showImage.sourceSize.width)
                    }
                }
                cache: true
                asynchronous: true
            }

            AnimatedImage {
                id: loadingAnimation
                anchors.fill: parent
                source: "qrc:/AoNami/resources/gifs/image-loading.gif"
                visible: showImage.status == Image.Loading
                playing: showImage.status == Image.Loading
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                cursorShape: Qt.PointingHandCursor
                onClicked: (mouse) => imageClicked(mouse)
            }
        }

        TextEdit {
            id: showTitleText
            anchors {
                top: imageClip.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: 4
            }
            height: 60 * Globals.fontSizeMultiplier - 4
            wrapMode: TextEdit.Wrap
            horizontalAlignment: Text.AlignHCenter
            readOnly: true
            selectByMouse: true
            cursorVisible: false
            clip: true
            font.pixelSize: Globals.sp(22)
            color: "white"
        }
    }
}

