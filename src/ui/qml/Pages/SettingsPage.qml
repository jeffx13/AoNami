import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AoNami
import QtCore
import "../Components"


Page {
    background: Rectangle { color: "#0B1220" }
    header: Label {
        text: qsTr("Settings")
        color: "#E5E7EB"
        font.pixelSize: 20
        padding: 12
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        background: Rectangle { color: "#0B1220" }

        ColumnLayout {
            id: rootCol
            anchors.fill: parent
            anchors.margins: 16
            spacing: 14

            // General card
            Rectangle {
                color: "#0F172A"
                radius: 12
                border.color: "#1F2937"
                border.width: 1
                Layout.fillWidth: true
                implicitHeight: generalCol.implicitHeight + 20
                ColumnLayout {
                    id: generalCol
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10
                    Text { text: qsTr("General"); color: "#A7F3D0"; font.pixelSize: 14 }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("MPV Logs"); color: "#E5E7EB"; Layout.fillWidth: true }
                        AppSwitch {
                            id: mpvLogSwitch
                            focusPolicy: Qt.NoFocus
                            checked: App.settings.mpvLogEnabled
                            onToggled: App.settings.mpvLogEnabled = checked
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        AppButton {
                            text: qsTr("Clear Logs")
                            onClicked: App.logList.clear()
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            // Network card
            Rectangle {
                color: "#0F172A"
                radius: 12
                border.color: "#1F2937"
                border.width: 1
                Layout.fillWidth: true
                implicitHeight: networkCol.implicitHeight + 20
                ColumnLayout {
                    id: networkCol
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10
                    Text { text: qsTr("Network"); color: "#A7F3D0"; font.pixelSize: 14 }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Proxy (http[s]://host:port)"); color: "#E5E7EB"; Layout.fillWidth: true }
                        AppTextField {
                            id: proxyField
                            text: App.settings.proxy
                            placeholderText: qsTr("http://127.0.0.1:7890")
                            Layout.preferredWidth: 360
                            onEditingFinished: App.settings.proxy = text
                        }
                        AppButton { text: qsTr("Apply"); onClicked: App.settings.proxy = proxyField.text }
                        AppButton { text: qsTr("Clear"); onClicked: { proxyField.text = ""; App.settings.proxy = "" } }
                    }
                }
            }

            // Player card
            Rectangle {
                color: "#0F172A"
                radius: 12
                border.color: "#1F2937"
                border.width: 1
                Layout.fillWidth: true
                implicitHeight: playerCol.implicitHeight + 20
                ColumnLayout {
                    id: playerCol
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10
                    Text { text: qsTr("Player"); color: "#A7F3D0"; font.pixelSize: 14 }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Use yt-dlp (mpv ytdl)"); color: "#E5E7EB"; Layout.fillWidth: true }
                        AppSwitch {
                            id: ytdlSwitch
                            focusPolicy: Qt.NoFocus
                            checked: App.settings.mpvYtdlEnabled
                            onToggled: App.settings.mpvYtdlEnabled = checked
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        AppButton {
                            text: qsTr("Open mpv folder")
                            onClicked: Qt.openUrlExternally(StandardPaths.writableLocation(StandardPaths.AppDataLocation) + "/../mpv")
                        }
                        AppButton {
                            text: qsTr("Open mpv config")
                            onClicked: Qt.openUrlExternally(StandardPaths.writableLocation(StandardPaths.AppDataLocation) + "/../mpv/mpv.conf")
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
