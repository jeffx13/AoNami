import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import "../components"

Popup  {
    parent: Overlay.overlay
    id:settingsRect



    background: Rectangle{
        radius: 10
        color: "green"
    }
    opacity: 0.8
    modal: true
    visible: false
    dim: false

    ColumnLayout {
        anchors.fill: parent
        Rectangle {
            color:"black"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 2
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: "qrc:/resources/images/player_settings.png"
                    Layout.fillHeight: true
                    Layout.preferredWidth: height
                    onClicked: {
                        stackView.replace(subtitleSetting)
                    }
                }
                ImageButton {
                    source: "qrc:/resources/images/player_settings.png"
                    Layout.fillHeight: true
                    Layout.preferredWidth: height
                    onClicked: {
                        stackView.replace(skipSetting)
                    }
                }
                ImageButton {
                    source: "qrc:/resources/images/player_settings.png"
                    Layout.fillHeight: true
                    Layout.preferredWidth: height
                    onClicked: {
                        stackView.replace(subtitleSetting)
                    }
                }
            }
        }

        StackView {
            id:stackView
            initialItem: subtitleSetting
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 8
        }
    }



    Component {
        id: subtitleSetting
        ListView {
            id: subtitlesListView
            model: app.play.subtitleList
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            anchors.fill: parent
            delegate: Rectangle {
                required property string label
                required property string file
                required property int index
                width: subtitlesListView.width
                height: 60 * root.fontSizeMultiplier
                color: app.play.subtitleList.currentIndex === index ? "purple" : "black"
                border.width: 2
                border.color: "white"
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
                    Text {
                        id: subtitleFileText
                        text: file
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
                    onClicked: app.play.subtitleList.currentIndex = index
                }
            }
        }
    }

    Component {
        id: skipSetting
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 8
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
            Text{
                Layout.row: 0
                Layout.column: 3
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 3
                text: "Enabled"
                color: "white"
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
                to: 180
                focusPolicy: Qt.NoFocus
                editable: true
                stepSize: 10
                onValueChanged: {
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
                to: 180
                focusPolicy: Qt.NoFocus
                editable: true
                stepSize: 10
                onValueChanged: {
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
                onCheckedChanged: root.mpv.shouldSkipOP = checked;
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
                to: 180
                focusPolicy: Qt.NoFocus
                editable: true
                stepSize: 10
                onValueChanged: {
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
                to: 180
                focusPolicy: Qt.NoFocus
                editable: true
                stepSize: 10
                onValueChanged: {
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
                onCheckedChanged: root.mpv.shouldSkipED = checked;
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
