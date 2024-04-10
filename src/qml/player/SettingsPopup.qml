import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts

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

    GridLayout {
        anchors.fill: parent
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
