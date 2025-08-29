import QtQuick
import QtQuick.Controls
import ".."
import QtQml
import Qt5Compat.GraphicalEffects

SpinBox {
	id: spinBox

	property color accentColor: "#4E5BF2"
	property color surfaceColor: "#0F172A"
	property color borderColor: "#2B2F44"
	property color hoverBorderColor: Qt.lighter(accentColor, 1.4)
	property color textColor: "#E5E7EB"
	property int fontSize: 20
	readonly property int scaledFontSize: Math.round(fontSize * (Globals.maximised ? fontScaleFactor : 1))
	property int fontScaleFactor: Globals.fontSizeMultiplier
	
	implicitHeight: 41
	implicitWidth: 120
	hoverEnabled: true
	editable: true
	leftPadding: 12
	rightPadding: 36
	inputMethodHints: Qt.ImhDigitsOnly


	

	contentItem: TextInput {
		id: input
		text: spinBox.textFromValue(spinBox.value, spinBox.locale)
		readOnly: !spinBox.editable
		selectByMouse: true
		validator: spinBox.validator
		inputMethodHints: Qt.ImhDigitsOnly
		font.pixelSize: spinBox.scaledFontSize
		font.weight: Font.Thin
		color: spinBox.textColor
		selectionColor: spinBox.accentColor
		selectedTextColor: spinBox.textColor
		verticalAlignment: Text.AlignVCenter
		horizontalAlignment: Text.AlignHCenter
	}

	up.indicator: Item {
		implicitWidth: 24
		implicitHeight: parent.height / 2
		anchors.right: parent.right
		anchors.rightMargin: 6
		anchors.top: parent.top
		Text {
			id: upArrow
			anchors.centerIn: parent
			text: "\u25B2"
			color: spinBox.up.pressed ? "#BFC7FF" : spinBox.textColor
			opacity: spinBox.enabled ? 1.0 : 0.5
			font.pixelSize: 11 * (Globals.maximised ? spinBox.fontScaleFactor : 1)
		}
	}

	down.indicator: Item {
		implicitWidth: 24
		implicitHeight: parent.height / 2
		anchors.right: parent.right
		anchors.rightMargin: 6
		anchors.bottom: parent.bottom
		Text {
			id: downArrow
			anchors.centerIn: parent
			text: "\u25BC"
			color: spinBox.down.pressed ? "#BFC7FF" : spinBox.textColor
			opacity: spinBox.enabled ? 1.0 : 0.5
			font.pixelSize: 11 * (Globals.maximised ? spinBox.fontScaleFactor : 1)
		}
	}

	background: Rectangle {
		id: bg
		color: surfaceColor
		radius: 12
		border.color: spinBox.activeFocus ? accentColor : (spinBox.hovered ? hoverBorderColor : borderColor)
		border.width: 1

		Behavior on border.color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
		Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

		layer.enabled: spinBox.hovered || spinBox.activeFocus
		layer.effect: DropShadow {
			transparentBorder: true
			color: Qt.rgba(78/255, 91/255, 242/255, spinBox.activeFocus ? 0.6 : 0.35)
			samples: spinBox.activeFocus ? 18 : 12
			horizontalOffset: 0
			verticalOffset: spinBox.activeFocus ? 6 : 3
		}
	}
}


