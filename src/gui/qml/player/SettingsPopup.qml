import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import Kyokou.App.Main

Popup  {
    id:settingsPopup
    modal: false
    dim: false
    background: Rectangle{
        radius: 12
        color: "#0F172A"
        border.color: "#1F2937"
        border.width: 1
    }
    opacity: 1.0
    property bool isOpen: false

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    property alias currentIndex: tabBar.currentIndex

    TabBar {
        id: tabBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            leftMargin: 2
            rightMargin: 2
        }
        height: Math.max(44, parent.height * 0.18)
        background: Rectangle { color: "#0B1220" }
        contentItem: ListView {
            id: view
            orientation: ListView.Horizontal
            boundsBehavior: Flickable.StopAtBounds
            model: ListModel {
                ListElement { text: qsTr("General") }
                ListElement { text: qsTr("Subtitles") }
                ListElement { text: qsTr("Skipping") }
            }
            delegate: Button {
                text: model.text
                width: Math.max(96, view.width / view.count)
                height: view.height
                onClicked: tabBar.currentIndex = index
                background: Rectangle {
                    radius: 10
                    color: tabBar.currentIndex === index ? "#162036" : "#0F172A"
                    border.color: tabBar.currentIndex === index ? "#4CAF50" : "#1F2937"
                    border.width: 1
                }
                contentItem: Text {
                    text: model.text
                    color: tabBar.currentIndex === index ? "#A7F3D0" : "#E5E7EB"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }


    }
    StackLayout {
        id:stackView
        width: parent.width
        anchors {
            top: tabBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        currentIndex: tabBar.currentIndex
        // General
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.margins: 10
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                spacing: 10
                Text { text: qsTr("Subtitles"); color: "#E5E7EB"; Layout.fillWidth: true }
                Switch {
                    id: subVisibleSwitch
                    focusPolicy: Qt.NoFocus
                    checked: mpv.subVisible
                    onToggled: mpv.subVisible = checked
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                spacing: 10
                Text { text: qsTr("Mute"); color: "#E5E7EB"; Layout.fillWidth: true }
                Switch {
                    id: muteSwitch
                    focusPolicy: Qt.NoFocus
                    checked: mpv.muted
                    onToggled: mpv.muted = checked
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                spacing: 10
                Text { text: qsTr("Volume"); color: "#E5E7EB"; Layout.fillWidth: true }
                Slider { from: 0; to: 100; value: mpv.volume; onMoved: mpv.volume = value; Layout.fillWidth: true }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                spacing: 10
                Text { text: qsTr("Speed"); color: "#E5E7EB"; Layout.fillWidth: true }
                Slider { from: 0.1; to: 4.0; stepSize: 0.05; value: mpv.speed; onMoved: mpv.speed = value; Layout.fillWidth: true }
            }
            Item { Layout.fillHeight: true }
        }

        // Subtitles
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.margins: 8
            spacing: 8
            Text { text: qsTr("Select Subtitle Track"); color: "#E5E7EB" }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0B1220"
                border.color: "#1F2937"
                border.width: 1
                radius: 8
                ListView {
                    anchors.fill: parent
                    id: subtitlesListView
                    model: mpv.subtitleList
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }
                    delegate: Button {
                        id: subItem
                        required property string title
                        required property int index
                        text: title
                        hoverEnabled: true
                        onClicked: mpv.setSubIndex(index)
                        background: Rectangle {
                            radius: 8
                            color: mpv.subtitleList.currentIndex === index ? "#1c2a22" : (subItem.hovered ? "#151924" : "#0f1115")
                            border.color: mpv.subtitleList.currentIndex === index ? "#21be2b" : "#1F2937"
                            border.width: 1
                            implicitHeight: 40
                        }
                        contentItem: Text {
                            text: title
                            color: mpv.subtitleList.currentIndex === index ? "#E6FFE8" : (subItem.hovered ? "#FFFFFF" : "#E0E0E0")
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                        }
                    }
                }
            }
        }

        // Skipping
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 4
            rows: 5
            focusPolicy: Qt.NoFocus
            Item {
                Layout.row: 0
                Layout.column: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
            }

            Text{
                Layout.row: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                Layout.column: 1
                text: "Start"
                color: "#E5E7EB"
            }
            Text{
                Layout.row: 0
                Layout.column: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                text: "Duration"
                color: "#E5E7EB"
            }
            Item {
                Layout.row: 0
                Layout.column: 3
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 3
            }


            Text{
                Layout.row: 1
                Layout.column: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "Skip OP"
                color: "#E5E7EB"
            }
            SpinBox{
                id: skipOPStart
                Layout.row: 1
                Layout.column: 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                value: 0
                from: 0
                to: mpv.duration
                focusPolicy: Qt.NoFocus
                editable: true
                stepSize: 10
                onValueModified: {
                    root.mpv.setSkipTimeOP(skipOPStart.value, skipOPLength.value)
                }
            }
            SpinBox{
                id: skipOPLength
                Layout.row: 1
                Layout.column: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                value: 90
                from: 0
                to: root.mpv.duration
                focusPolicy: Qt.NoFocus
                editable: true
                stepSize: 10
                onValueModified: {
                    root.mpv.setSkipTimeOP(skipOPStart.value, skipOPLength.value)
                }
            }
            CheckBox{
                id:skipOPCheckBox
                Layout.row: 1
                Layout.column: 3
                Layout.fillWidth: true
                Layout.fillHeight: true
                focusPolicy: Qt.NoFocus
                checked: root.mpv.shouldSkipOP
                onCheckedChanged: {
                    root.mpv.shouldSkipOP = checked;
                    root.mpv.setSkipTimeOP(skipOPStart.value, skipOPLength.value)
                }

            }

            Text{
                Layout.row: 2
                Layout.column: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "Skip ED"
                color: "#E5E7EB"
            }
            SpinBox{
                Layout.row: 2
                Layout.column: 1
                id: skipEDStart
                Layout.fillWidth: true
                Layout.fillHeight: true
                value: 0
                from: 0
                to: root.mpv.duration
                focusPolicy: Qt.NoFocus
                editable: true
                stepSize: 10
                onValueModified: {
                    root.mpv.setSkipTimeED(skipEDStart.value, skipEDLength.value)
                }

            }
            SpinBox{
                id: skipEDLength
                Layout.row: 2
                Layout.column: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                value: 90
                from: 0
                to: root.mpv.duration
                focusPolicy: Qt.NoFocus
                editable: true
                stepSize: 10
                onValueModified: {
                    root.mpv.setSkipTimeED(skipEDStart.value, skipEDLength.value)
                }
            }
            CheckBox{
                id:skipEDCheckBox
                Layout.row: 2
                Layout.column: 3
                Layout.fillWidth: true
                Layout.fillHeight: true
                focusPolicy: Qt.NoFocus
                checked: root.mpv.shouldSkipED
                onCheckedChanged: {
                    root.mpv.shouldSkipED = checked;
                    root.mpv.setSkipTimeED(skipEDStart.value, skipEDLength.value)
                }
            }


            Text{
                Layout.row: 3
                Layout.column: 0
                text: "Auto Skip"
                color: "#E5E7EB"
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            CheckBox{
                Layout.row: 3
                Layout.column: 1
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.fillHeight: true
                focusPolicy: Qt.NoFocus
                enabled: skipOPCheckBox.checked || skipEDCheckBox.checked
                checked: skipOPCheckBox.checked && skipEDCheckBox.checked
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Enable both Skip OP and Skip ED to auto-skip segments")
            }

            Text{
                Layout.row: 4
                Layout.column: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "Speed:"
                color: "#E5E7EB"
            }
            RowLayout {
                Layout.row: 4
                Layout.column: 1
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.fillHeight: true
                Slider { from: 0.25; to: 3.0; stepSize: 0.05; value: mpv.speed; onMoved: mpv.speed = value; Layout.fillWidth: true }
                Text { text: mpv.speed.toFixed(2) + "x"; color: "#E5E7EB" }
            }
        }


    }










}
