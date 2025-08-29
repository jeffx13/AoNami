import QtQuick
import QtQuick.Controls
import ".."
import Qt5Compat.GraphicalEffects

CheckBox {
	id: control

	// Theme
	property color accentColor: "#4E5BF2"
	property color surfaceColor: "#0F172A"
	property color borderColor: "#2B2F44"
	property color hoverBorderColor: Qt.lighter(accentColor, 1.4)
	property color textColor: "#E5E7EB"

	// Typography / scaling
	property int fontSize: 20
	readonly property int scaledFontSize: Math.round(fontSize * (Globals.maximised ? fontScaleFactor : 1))
	property int fontScaleFactor: Globals.fontSizeMultiplier

	// Sizing
	property int indicatorSize: 22
	implicitHeight: Math.max(indicatorSize + 6, scaledFontSize + 12)
	implicitWidth: 140
	leftPadding: indicatorSize + 12
	rightPadding: 12
	spacing: 8
	hoverEnabled: true

	indicator: Rectangle {
		id: indicator
		implicitWidth: control.indicatorSize
		implicitHeight: control.indicatorSize
		x: control.leftPadding
		y: parent.height / 2 - height / 2
		radius: 6
		color: control.surfaceColor
		border.color: control.checked ? control.accentColor : (control.hovered ? control.hoverBorderColor : control.borderColor)
		border.width: 1

		Behavior on border.color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
		Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

		layer.enabled: control.hovered || control.activeFocus
		layer.effect: DropShadow {
			transparentBorder: true
			color: Qt.rgba(78/255, 91/255, 242/255, control.activeFocus ? 0.6 : 0.35)
			samples: control.activeFocus ? 18 : 12
			horizontalOffset: 0
			verticalOffset: control.activeFocus ? 6 : 3
		}

		Text {
			anchors.centerIn: parent
			text: "✔"
			font.pixelSize: Math.round(parent.height * 0.8)
			color: control.accentColor
			visible: control.checked
			opacity: control.enabled ? 1.0 : 0.5
		}
	}

	contentItem: Text {
		text: control.text
		font.pixelSize: control.scaledFontSize
		font.weight: Font.Thin
		color: control.enabled ? control.textColor : Qt.lighter(control.textColor, 1.4)
		horizontalAlignment: Text.AlignLeft
		verticalAlignment: Text.AlignVCenter
		leftPadding: control.indicator.width + control.spacing
	}
}


