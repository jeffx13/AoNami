pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Kyokou 
import QtQuick.Layouts
import "../Components"
import ".."
Item {
    id: logPage

    Component.onCompleted: logListView.positionViewAtEnd()

    Rectangle {
        id: topBarCard
        height: Math.max(56, parent.height * 0.08)
        radius: 12
        color: "#0f1324cc"
        border.color: "#334E5BF2"
        border.width: 1

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 12
            leftMargin: 12
            rightMargin: 12
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Text {
                text: "Logs"
                color: "#e8ebf6"
                font.pixelSize: Globals.sp(22)
                Layout.fillHeight: true
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
            }

            AppButton {
                id: clearButton
                text: "Clear"
                fontSize: 22
                radius: 12
                Layout.fillHeight: true
                Layout.preferredWidth: 100
                onClicked: App.logList.clear()
            }
        }
    }

    ListView {
        id: logListView
        anchors {
            left : parent.left
            right: parent.right
            top: topBarCard.bottom
            bottom: parent.bottom
            leftMargin: 12
            rightMargin: 12
            topMargin: 10
            bottomMargin: 12
        }

        model: App.logList
        clip: true
        spacing: 10
        cacheBuffer: 20000

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            parent: logListView.parent
            width: 8
            anchors {
                top: logListView.top
                left: logListView.right
                bottom: logListView.bottom
            }
            contentItem: Rectangle {
                color: "#2F3B56"
                radius: width / 2
            }
            background: Rectangle {
                color: "#121826"
                radius: width / 2
            }
        }

        onCountChanged: {
            if (atYEnd || contentY >= contentHeight - height - 100) positionViewAtEnd()
        }

        delegate: Item {
            id: logItem
            width: logListView.width
            implicitHeight: card.implicitHeight
            required property string message
            required property string time
            required property string type
            required property string colour

            Rectangle {
                id: card
                width: parent.width
                radius: 12
                color: "#0f1324cc"
                border.color: "#334E5BF2"
                border.width: 1
                implicitHeight: contentColumn.implicitHeight + 24

                anchors {
                    left: parent.left
                    right: parent.right
                }

                ColumnLayout {
                    id: contentColumn
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    RowLayout {
                        spacing: 10
                        Layout.fillWidth: true

                        Text {
                            id: timeText
                            text: logItem.time
                            color: "#a3aed0"
                            font.pixelSize: Globals.sp(18)
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Rectangle {
                            id: typeBadge
                            radius: 8
                            color: logItem.colour
                            border.color: logItem.colour
                            border.width: 1
                            Layout.preferredHeight: 22
                            Layout.minimumWidth: typeText.implicitWidth + 14

                            Text {
                                id: typeText
                                anchors.centerIn: parent
                                text: logItem.type
                                color: "#fff"
                                font.pixelSize: Globals.sp(16)
                            }
                        }
                    }

                    TextEdit {
                        id: logText
                        readOnly: true
                        selectByMouse: true
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.Wrap
                        horizontalAlignment: TextEdit.AlignLeft
                        verticalAlignment: TextEdit.AlignTop
                        text: logItem.message
                        font.pixelSize: Globals.sp(20)
                        color: logItem.colour
                        Layout.fillWidth: true
                        onLinkActivated: (link) => Qt.openUrlExternally(link)

                        HoverHandler {
                            enabled: logText.hoveredLink
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                }
            }
        }
    }
}
