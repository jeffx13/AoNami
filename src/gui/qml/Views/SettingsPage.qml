import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    header: Label {
        text: qsTr("Settings")
        font.pixelSize: 20
        padding: 10
    }
    ColumnLayout {
        anchors.fill: parent
        Row {
            Layout.preferredHeight: 20
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: "General"
            }
            Text {
                text: "Proxy"
            }
            Text {
                text: "Player"
            }
        }
        Row {
            Layout.fillHeight: true
            Layout.fillWidth: true
            TextField {

            }

        }

        SwipeView {
            clip: true
            Layout.fillHeight: true
            Layout.fillWidth: true
            Item {
                Rectangle {
                    color: "red"
                    anchors.fill: parent
                }
            }

            Item {
                Rectangle {
                    color: "orange"
                    anchors.fill: parent
                }
            }
        }

    }



}
