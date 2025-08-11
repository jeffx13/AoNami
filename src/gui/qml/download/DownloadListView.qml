import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import Kyokou.App.Main
ListView {
    id: listView
    clip: true
    boundsMovement: Flickable.StopAtBounds
    spacing: 10
    signal downloadCancelled(int index)
    delegate: Rectangle {
        required property int progressValue;
        required property string progressText;
        required property string downloadName;
        required property string downloadPath;
        required property int index;
        width: listView.width
        height: 150
        border.width: 3
        border.color: "white"
        color: "black"
        id: taskDelegate

        GridLayout {
            anchors.fill: parent
            anchors.margins: 5
            rows:4
            columns: 3
            rowSpacing: 10
            Text {
                Layout.row: 0
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.preferredWidth: 8
                id: nameStr
                text:  taskDelegate.downloadName
                font.pixelSize: 20 * root.fontSizeMultiplier
                elide: Text.ElideRight
                color: "white"
            }
            Text {
                Layout.row: 1
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.preferredWidth: 8
                elide: Text.ElideRight
                id: pathStr
                text: taskDelegate.downloadPath
                font.pixelSize: 20 * root.fontSizeMultiplier

                color: "white"

            }

            CustomButton{
                Layout.row: 1
                Layout.column: 2
                Layout.rowSpan: 2
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                text: "Cancel"
                onClicked: listView.downloadCancelled(taskDelegate.index)
            }


            ProgressBar {
                Layout.row: 2
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.preferredWidth: 8
                from: 0
                to: 100
                value: taskDelegate.progressValue
                indeterminate: value === 0
            }
            Text {
                Layout.row: 3
                Layout.column: 0
                Layout.fillWidth: true
                Layout.columnSpan: 2
                Layout.preferredWidth: 8
                text:  taskDelegate.progressText
                font.pixelSize: 20 * root.fontSizeMultiplier
                elide: Text.ElideRight
                color: "white"
            }
        }


    }
}
