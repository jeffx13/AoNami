import QtQuick
import QtQuick.Controls
import ".."

Popup {
    property alias backgroundRadius: bg.radius

    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: Theme.scrim }

    enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 120; easing.type: Easing.OutCubic } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 90;  easing.type: Easing.InCubic  } }

    background: Card { id: bg }
}
