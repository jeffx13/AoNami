import QtQuick
import QtQuick.Controls
import ".."

AbstractButton {
    id: btn

    // Not `icon`: AbstractButton declares that as a final grouped property.
    property string iconName: ""
    property string tip: ""
    property int    iconSize: 20
    property real   boxRadius: 9
    property color  hoverColor: Theme.border
    property color  iconColor: Theme.textSecondary
    property color  iconHoverColor: Theme.textPrimary
    property bool   active: false        // paint hoverColor even while not hovered

    implicitWidth: 36
    implicitHeight: 36
    focusPolicy: Qt.NoFocus
    hoverEnabled: true

    background: Rectangle {
        radius: btn.boxRadius
        color: btn.active || btn.hovered ? btn.hoverColor : "transparent"
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    contentItem: Item {
        AppIcon {
            anchors.centerIn: parent
            name: btn.iconName
            size: btn.iconSize
            color: btn.hovered ? btn.iconHoverColor : btn.iconColor
        }
    }

    AppToolTip { text: btn.tip; visible: btn.tip !== "" && btn.hovered }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
