import QtQuick


Item {
    property alias title: showTitle.text
    property alias cover: showImage.source
    // readonly property real imageAspectRatio: 319/225

    Image {
        id: showImage
        // source:  cover
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 2
        }
        height: width * (319/225)
        onStatusChanged: {
            // if (showImage.status != Image.Loading) loadingAnimation.destroy()
            if (showImage.status === Image.Error) source = "qrc:/resources/images/error_image.png"
        }
        AnimatedImage {
            id: loadingAnimation
            anchors {
                left:parent.left
                right:parent.right
                bottom:parent.bottom
            }
            source: "qrc:/resources/gifs/image-loading.gif"
            width: parent.width
            height: width * 0.84
            visible: parent.status == Image.Loading
            playing: parent.status == Image.Loading

        }
    }

    Text {
        id: showTitle
        anchors {
            top: showImage.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 2
        }
        height: 60 * root.fontSizeMultiplier - 4
        horizontalAlignment:Text.AlignHCenter
        wrapMode: Text.Wrap
        font.pixelSize: 22 * root.fontSizeMultiplier
        elide: Text.ElideRight
        color: "white"
    }
}

