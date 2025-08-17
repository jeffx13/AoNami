import QtQuick
import Kyokou.App.Main // qmllint disable
import QtQuick.Layouts
Item {
    id: logPage

    Component.onCompleted: {
        logListView.positionViewAtEnd()
    }
    AppButton {
        id: clearButton
        anchors {
            top : parent.top
            right: parent.right
        }
        height: 40
        width: 80
        text: "Clear"
        onClicked: {
            App.logList.clear()
        }
    }

    ListView {
        id: logListView
        anchors {
            left : parent.left
            right: parent.right
            top: clearButton.bottom
            bottom: parent.bottom
        }

        model: App.logList
        clip: true

        delegate: RowLayout {
            width: logListView.width
            height: logListView.height / 10
            Rectangle {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 0.15

                color: "transparent"
                border.color: "white"
                Text {
                    id: timeText
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                    }
                    height: parent.height / 2
                    text: model.time
                    // align centre
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 20
                    color: model.colour
                    font.bold: true
                }
                Text {
                    id: typeText
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: timeText.bottom
                        bottom: parent.bottom
                    }
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    height: parent.height / 2
                    text: model.type
                    font.pixelSize: 20
                    color: model.colour
                    font.bold: true
                }

            }

            Text {
                id: logText
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 0.85

                linkColor: "cyan"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                textFormat: Text.StyledText
                text: model.message
                font.pixelSize: 20
                color: model.colour
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                onLinkActivated: link => Qt.openUrlExternally(link)
                HoverHandler {
                    enabled: parent.hoveredLink
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }
}
