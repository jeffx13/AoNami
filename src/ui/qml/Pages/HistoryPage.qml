import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AoNami
import "../Components"
import ".."

Item {
    id: historyPage

    function refresh() { historyList.model = App.library.history() }

    onVisibleChanged: if (visible) refresh()
    Component.onCompleted: refresh()
    Connections { target: App.library; function onHistoryChanged() { historyPage.refresh() } }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("History")
                color: Theme.textPrimary
                font.pixelSize: Globals.sp(26)
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Text {
                text: historyList.count + (historyList.count === 1 ? " show" : " shows")
                color: Theme.textMuted
                font.pixelSize: Globals.sp(20)
            }

            AppButton {
                text: qsTr("Clear")
                fontSize: 20
                cornerRadius: 8
                backgroundDefaultColor: Theme.surfaceAlt
                contentItemTextColor: Theme.danger
                visible: historyList.count > 0
                Layout.preferredHeight: 34
                Layout.preferredWidth: 78
                onClicked: App.library.clearHistory()
            }
        }

        Text {
            visible: historyList.count === 0
            Layout.fillWidth: true
            Layout.topMargin: 48
            text: qsTr("Nothing watched yet.")
            color: Theme.textMuted
            font.pixelSize: Globals.sp(22)
            horizontalAlignment: Text.AlignHCenter
        }

        ListView {
            id: historyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded; width: 8
                contentItem: Rectangle { radius: 4; color: Theme.textMuted; opacity: 0.6 }
            }

            delegate: ItemDelegate {
                id: row
                required property int index
                required property var modelData
                width: ListView.view.width
                height: 84
                focusPolicy: Qt.NoFocus

                background: Rectangle {
                    radius: 12
                    color: row.hovered ? Qt.alpha(Theme.accent, 0.08) : Theme.surface
                    border.color: row.hovered ? Qt.alpha(Theme.accent, 0.30) : Theme.border
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                onClicked: App.resumeFromHistory(row.modelData.link)

                contentItem: RowLayout {
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 50
                        Layout.preferredHeight: 68
                        radius: 6
                        color: Theme.surfaceDeep
                        clip: true
                        Image {
                            anchors.fill: parent
                            source: row.modelData.cover
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            text: row.modelData.title
                            color: Theme.textPrimary
                            font.pixelSize: Globals.sp(21)
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: row.modelData.episode > 0
                                  ? qsTr("Episode ") + row.modelData.episode
                                    + (row.modelData.total > 0 ? " / " + row.modelData.total : "")
                                  : qsTr("Not started")
                            color: Theme.textMuted
                            font.pixelSize: Globals.sp(19)
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 4
                            radius: 2
                            color: Theme.surfaceAlt
                            Rectangle {
                                width: parent.width * row.modelData.progress
                                height: parent.height
                                radius: 2
                                color: Theme.accent
                            }
                        }
                    }

                    AppIcon {
                        name: "play"
                        size: 22
                        color: row.hovered ? Theme.accent : Theme.textMuted
                    }
                }
            }
        }
    }
}
