import QtQuick 2.15
import QtQuick.Controls 2.15
import "./../components"
import QtQuick.Layouts 1.15
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
            CustomTextField {
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
            CustomButton {
                Layout.row: 0
                Layout.column: 1
                text: "Browse"
                onClicked: folderDialog.open()
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
            }
            CustomButton {
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
            CustomTextField {
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

            CustomTextField {
                id: downloadUrlField
                checkedColor: "#727CF5"
                color: "white"
                placeholderText: qsTr("Enter m3u8 link")
                placeholderTextColor: "gray"
                font.pixelSize: 20
                Layout.row: 1
                Layout.column: 1
                Layout.fillWidth: true
                Layout.preferredWidth: 7
                Layout.fillHeight: true
                onAccepted: downloadPage.download()
            }

            CustomButton{
                Layout.row: 1
                Layout.column: 2
                text: "Download"
                onClicked: downloadPage.download()
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
            }
        }

        DownloadListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 8.5
            model: App.downloader
            onDownloadCancelled: (index) => App.downloader.cancelTask(index)
        }

    }






}

