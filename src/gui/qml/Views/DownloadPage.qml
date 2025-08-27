import QtQuick
import QtQuick.Controls
import "./../Components"
import QtQuick.Layouts
import QtQuick.Dialogs
import Kyokou.App.Main
Item {
    id:downloadPage

    FolderDialog {
        id:folderDialog
        currentFolder: "file:///" + workDirTextField.text
        onAccepted: {
            App.downloader.workDir = text
            text = App.downloader.workDir
        }
    }
    function download(){
        if (downloadNameField.text === "" || downloadUrlField.text === "") {
            return
        }
        App.downloader.downloadLink(downloadNameField.text, downloadUrlField.text)
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }
        spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 5
            Layout.preferredHeight: 1
            AppTextField {
                id: workDirTextField
                text: App.downloader.workDir
                checkedColor: "#727CF5"
                color: "white"
                placeholderText: qsTr("Enter working directory")
                placeholderTextColor: "gray"
                fontSize: 20
                onAccepted: () => {
                                App.downloader.workDir = text
                                text = App.downloader.workDir
                            }

                Layout.row: 0
                Layout.column: 0
                Layout.fillWidth: true
                Layout.preferredWidth: 8
                Layout.fillHeight: true
            }
            AppButton {
                Layout.row: 0
                Layout.column: 1
                text: "Browse"
                onClicked: folderDialog.open()
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
            }
            AppButton {
                text: "Open"
                onClicked: Qt.openUrlExternally("file:///" + App.downloader.workDir)
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.fillHeight: true
            spacing: 5
            AppTextField {
                id: downloadNameField
                checkedColor: "#727CF5"
                color: "white"
                placeholderText: qsTr("Enter filename")
                placeholderTextColor: "gray"
                font.pixelSize: 20
                Layout.row: 1
                Layout.column: 0
                Layout.fillWidth: true
                Layout.preferredWidth: 3
                Layout.fillHeight: true
                onAccepted: downloadPage.download()
            }

            AppTextField {
                id: downloadUrlField
                checkedColor: "#727CF5"
                color: "white"
                placeholderText: qsTr("Enter m3u8 link")
                placeholderTextColor: "gray"
                font.pixelSize: 20
                Layout.row: 1
                Layout.column: 1
                Layout.fillWidth: true
                Layout.preferredWidth: 6
                Layout.fillHeight: true
                onAccepted: downloadPage.download()
            }

            AppButton{
                Layout.row: 1
                Layout.column: 2
                text: "Download"
                onClicked: downloadPage.download()
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
            }
            // SpinBox {
            //     id: endSpinBox
            //     value: App.downloader.maxDownloads
            //     from : 1
            //     to: 8
            //     onValueModified: {
            //         App.downloader.maxDownloads = value
            //     }
            //     editable:true
            //     Layout.fillWidth: true
            //     Layout.preferredWidth: 1
            // }
        }



        ListView {
            id: listView
            clip: true
            boundsMovement: Flickable.StopAtBounds
            spacing: 10
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 8.5
            model: App.downloadListModel

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

                    AppButton{
                        Layout.row: 1
                        Layout.column: 2
                        Layout.rowSpan: 2
                        Layout.fillWidth: true
                        Layout.preferredWidth: 2
                        text: "Cancel"
                        onClicked: App.downloader.cancelTask(taskDelegate.index)
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


    }






}

