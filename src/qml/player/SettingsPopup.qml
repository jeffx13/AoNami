// qmllint disable unqualified import
import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import Kyokou.App.Main
Popup  {
    id:settingsPopup
    // parent: Overlay.overlay
    modal: false
    dim: false
    background: Rectangle{
        radius: 10
        color: "black"
    }
    opacity: 0.8
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
        height: parent.height * 0.15
        contentItem: ListView {
                    id: view
                    model: ListModel {
                        ListElement { text: qsTr("General") }
                        ListElement { text: qsTr("Subs") }
                        ListElement { text: qsTr("Skip") }
                    }
                    orientation: ListView.Horizontal
                    delegate: Button {
                        text: model.text
                        width: view.width / view.count
                        height: view.height
                        onClicked: tabBar.currentIndex = index
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
        Item {
            Text {
                text: "General"
                color: "white"
            }
        }

        Item {
            Text {
                id: subsText
                text: "Subs"
                color: "white"
                font.pixelSize: 25 * root.fontSizeMultiplier
                height: 30 * root.fontSizeMultiplier
                horizontalAlignment: Text.AlignHCenter
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
            }

            Rectangle {
                anchors {
                    top: subsText.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                border.color: "white"
                border.width: 2
                color: "black"
                ListView {
                    anchors.fill: parent
                    id: subtitlesListView
                    model: App.play.subtitleList
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Rectangle {
                        required property string label
                        required property string file
                        required property int index
                        width: subtitlesListView.width
                        height: 30 * root.fontSizeMultiplier
                        color: App.play.subtitleList.currentIndex === index ? "purple" : "black"
                        border.width: 2
                        border.color: "cyan"
                        ColumnLayout {
                            anchors {
                                fill: parent
                                margins: 3
                            }
                            Text {
                                id: subtitleLabelText
                                text: label
                                font.pixelSize: 25 * root.fontSizeMultiplier
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                elide: Text.ElideRight
                                wrapMode: Text.Wrap
                                color: "white"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: App.play.subtitleList.currentIndex = index
                        }
                    }
                }

            }
        }

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
                color: "white"
            }
            Text{
                Layout.row: 0
                Layout.column: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                text: "Duration"
                color: "white"
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
                color: "white"
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
                color: "white"
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
                color: "white"
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
            }

            Text{
                Layout.row: 4
                Layout.column: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "Speed:"
                color: "white"
            }
            SpinBox{
                Layout.row: 4
                Layout.column: 1
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.fillHeight: true
                focusPolicy: Qt.NoFocus
                value: mpv.speed * decimalFactor
                editable: true
                stepSize: decimalFactor/10
                from: 0
                to: 1000

                onValueModified: {
                    mpv.speed = value / decimalFactor
                }
                id:spinBox
                property int decimals: 1
                readonly property int decimalFactor: Math.pow(10, decimals)

                textFromValue: function(value, locale) {
                    return Number(value / decimalFactor).toLocaleString(locale, 'f', spinBox.decimals)
                }

                valueFromText: function(text, locale) {
                    return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                }
            }
        }


    }










}
