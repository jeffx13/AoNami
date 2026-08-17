import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AoNami
import "../Components"
import ".."

Item {
    id: historyPage

    function refresh() {
        const keep = historyList.currentIndex
        historyList.model = App.library.history()
        historyList.currentIndex = Math.min(Math.max(0, keep), historyList.count - 1)
    }

    focus: true
    onVisibleChanged: if (visible) { refresh(); historyList.forceActiveFocus() }
    onActiveFocusChanged: if (activeFocus) historyList.forceActiveFocus()
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
            focus: true
            keyNavigationEnabled: true
            keyNavigationWraps: true
            currentIndex: 0
            highlightMoveDuration: 120
            preferredHighlightBegin: height / 3
            preferredHighlightEnd: height * 2 / 3
            highlightRangeMode: ListView.ApplyRange

            ScrollBar.vertical: AppScrollBar {}

            function playAt(i) {
                if (i >= 0 && i < count) App.resumeFromHistory(model[i].link)
            }
            function removeAt(i) {
                if (i >= 0 && i < count) App.library.removeFromHistory(model[i].link)
            }

            Keys.onPressed: function (event) {
                switch (event.key) {
                case Qt.Key_Return:
                case Qt.Key_Enter:
                case Qt.Key_Space:
                    playAt(currentIndex); event.accepted = true; break
                case Qt.Key_Delete:
                case Qt.Key_Backspace:
                    removeAt(currentIndex); event.accepted = true; break
                case Qt.Key_Home:
                    currentIndex = 0; event.accepted = true; break
                case Qt.Key_End:
                    currentIndex = count - 1; event.accepted = true; break
                }
            }

            delegate: ItemDelegate {
                id: row
                required property int index
                required property var modelData
                readonly property bool current: ListView.isCurrentItem && ListView.view.activeFocus
                width: ListView.view.width
                height: 84
                focusPolicy: Qt.NoFocus

                background: Rectangle {
                    radius: 12
                    color: row.current  ? Qt.alpha(Theme.accent, 0.14)
                         : row.hovered  ? Qt.alpha(Theme.accent, 0.08) : Theme.surface
                    border.color: row.current ? Theme.accent
                                : row.hovered ? Qt.alpha(Theme.accent, 0.30) : Theme.border
                    border.width: row.current ? 2 : 1
                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                onClicked: { row.ListView.view.currentIndex = row.index; App.resumeFromHistory(row.modelData.link) }

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
                            Layout.preferredHeight: 4
                            radius: 2
                            color: Theme.surfaceAlt
                            Rectangle {
                                width: parent.width * Math.max(0, Math.min(1, row.modelData.progress || 0))
                                height: parent.height
                                radius: 2
                                color: Theme.accent
                            }
                        }
                    }

                    AppIcon {
                        name: "play"
                        size: 22
                        color: row.current || row.hovered ? Theme.accent : Theme.textMuted
                    }

                    Item {
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        opacity: row.hovered || row.current ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }

                        Rectangle {
                            anchors.fill: parent
                            radius: 6
                            color: removeArea.containsMouse ? Qt.alpha(Theme.danger, 0.18) : "transparent"
                        }
                        AppIcon {
                            anchors.centerIn: parent
                            name: "x"
                            size: 16
                            color: removeArea.containsMouse ? Theme.danger : Theme.textMuted
                        }
                        MouseArea {
                            id: removeArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: App.library.removeFromHistory(row.modelData.link)
                        }
                        AppToolTip { visible: removeArea.containsMouse; text: qsTr("Remove from history (Del)") }
                    }
                }
            }
        }
    }
}
