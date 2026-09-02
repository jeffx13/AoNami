import QtQuick
import QtQuick.Layouts
import ".."

// Declared children land after the label column: `data` is the default property.
RowLayout {
    id: row

    property string label: ""
    property string sublabel: ""

    Layout.fillWidth: true
    spacing: 8

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Text {
            text: row.label
            color: Theme.textSecondary
            font.pixelSize: Globals.sp(20)
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Text {
            text: row.sublabel
            visible: row.sublabel !== ""
            color: Theme.textMuted
            font.pixelSize: Globals.sp(15)
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
