import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ListView {
    id: listView
    clip: true
    boundsMovement: Flickable.StopAtBounds
    spacing: 10
    delegate: Rectangle {
        required property int progressValue;
        required property string progressText;
        required property string name;
        required property string path;
        required property int index;
        width: listView.width
        height: 120
        border.width: 3
        border.color: "white"
        color: "black"

        GridLayout {
            anchors {
                fill: parent
                margins: parent.border.width + 2
            }
            rows:4
            columns: 3
            rowSpacing: 10
            Text {
                Layout.row: 0
                Layout.column: 0
                Layout.columnSpan: 2
                id: nameStr
                text:  name
                font.pixelSize: 20 * root.fontSizeMultiplier

                wrapMode: Text.Wrap
                color: "white"
            }
            Text {
                Layout.row: 1
                Layout.column: 0
                Layout.columnSpan: 2
                id: pathStr
                text: path
                font.pixelSize: 20 * root.fontSizeMultiplier
                wrapMode: Text.Wrap
                color: "white"
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: {
                        Qt.openUrlExternally("file:///" + path)
                        // app.downloader.openFolder(path);
                    }
                }
            }

            CustomButton{
                Layout.row: 0
                Layout.column: 2
                Layout.rowSpan: 4
                text: "Cancel"
                onClicked: app.downloader.cancelTask(index)
            }


            ProgressBar {
                Layout.row: 2
                Layout.column: 0
                Layout.columnSpan: 2
                from: 0
                to: 100
                value: progressValue

                Layout.fillWidth: true
            }
            Text {
                Layout.row: 3
                Layout.column: 0
                text:  progressText
                font.pixelSize: 20 * root.fontSizeMultiplier
                Layout.columnSpan: 2
                wrapMode: Text.Wrap
                color: "white"
            }
        }


    }
}
