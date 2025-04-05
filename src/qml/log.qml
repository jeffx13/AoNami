import QtQuick
import Kyokou.App.Main // qmllint disable

Item {
    id: logPage

    Component.onCompleted: {
        //scrollToBottom()
        logListView.positionViewAtEnd()
    }

    ListView {
        id: logListView
        anchors.fill: parent
        model: App.logList
        clip: true  // ensures overflowing content is not drawn

        delegate: Item {
            width: logListView.width
            height: logText.implicitHeight  // dynamic height based on text

            Text {
                id: logText
                textFormat: Text.RichText
                text: model.message
                font.pixelSize: 20
                color: "white"
                wrapMode: Text.Wrap
                width: parent.width
                elide: Text.ElideRight
                onLinkActivated: link => Qt.openUrlExternally(link)
            }
        }
    }
}
