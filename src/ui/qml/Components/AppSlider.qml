import QtQuick
import QtQuick.Controls
import ".."
import Qt5Compat.GraphicalEffects

Slider {
	id: control

	// Theme
	property color accentColor: "#4E5BF2"
	property color surfaceColor: "#0F172A"
	property color trackColor: "#2F3B56"
	property color textColor: "#E5E7EB"
	property color borderColor: "#2B2F44"
	property color hoverBorderColor: Qt.lighter(accentColor, 1.4)

	// Typography / scaling
	property int fontSize: 20
	readonly property int scaledFontSize: Math.round(fontSize * (Globals.maximised ? fontScaleFactor : 1))
	property int fontScaleFactor: Globals.fontSizeMultiplier

	// Display
	property int decimals: 0
	property string unitSuffix: ""
	property bool showValueBubble: true

	implicitHeight: 36
	implicitWidth: 140
	hoverEnabled: true
	focusPolicy: Qt.NoFocus
	live: true

	// Ensure vertical centering: paddings based on handle size
	topPadding: handle.height / 2
	bottomPadding: handle.height / 2

	leftPadding: Math.max(handle.width / 2, 10)
	rightPadding: Math.max(handle.width / 2, 10)

	background: Rectangle {
		id: track
		x: control.leftPadding
		y: control.topPadding + control.availableHeight / 2 - height / 2
		width: control.availableWidth
		implicitHeight: 10 * (Globals.maximised ? control.fontScaleFactor : 1) * 0.75
		height: implicitHeight
		radius: height / 2
		color: control.enabled ? control.trackColor : Qt.lighter(control.trackColor, 1.2)
		// Subtle vertical gradient for depth
		gradient: Gradient {
			GradientStop { position: 0.0; color: Qt.darker(control.trackColor, 1.25) }
			GradientStop { position: 1.0; color: Qt.lighter(control.trackColor, 1.15) }
		}
		border.color: control.hovered ? control.hoverBorderColor : control.borderColor
		border.width: 1

		Behavior on border.color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
		Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

		Rectangle {
			id: progress
			width: control.visualPosition * parent.width
			height: parent.height
			radius: height / 2
			color: control.accentColor
			// Shiny gradient fill to match accent
			gradient: Gradient {
				GradientStop { position: 0.0; color: Qt.lighter(control.accentColor, 1.35) }
				GradientStop { position: 0.5; color: control.accentColor }
				GradientStop { position: 1.0; color: Qt.darker(control.accentColor, 1.25) }
			}
		}

		// Gloss highlight on top portion of the track
		Rectangle {
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.top: parent.top
			height: Math.max(2, Math.round(parent.height * 0.45))
			radius: height / 2
			color: "transparent"
			gradient: Gradient {
				GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.22) }
				GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.0) }
			}
		}

		layer.enabled: control.hovered || control.activeFocus
		layer.effect: DropShadow {
			transparentBorder: true
			color: Qt.rgba(78/255, 91/255, 242/255, control.activeFocus ? 0.6 : 0.35)
			samples: control.activeFocus ? 18 : 12
			horizontalOffset: 0
			verticalOffset: control.activeFocus ? 6 : 3
		}
	}

	handle: Rectangle {
		id: handle
		width: 18 * (Globals.maximised ? control.fontScaleFactor : 1)
		height: width
		radius: width / 2
		color: control.pressed ? "#f0f0f0" : "#f6f6f6"
		// Soft glossy gradient on handle
		gradient: Gradient {
			GradientStop { position: 0.0; color: control.pressed ? "#f2f2f2" : "#ffffff" }
			GradientStop { position: 1.0; color: control.pressed ? "#e9e9e9" : "#eaeaea" }
		}
		border.color: "#bdbebf"
		x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
		y: control.topPadding + control.availableHeight / 2 - height / 2

		scale: control.pressed || control.hovered ? 1.1 : 1.0

		Behavior on x { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
		Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutBack } }
		Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

		MouseArea {
			anchors.fill: parent
			acceptedButtons: Qt.NoButton
			cursorShape: Qt.PointingHandCursor
		}

		// Subtle glow when interacting
		layer.enabled: control.hovered || control.pressed
		layer.effect: DropShadow {
			transparentBorder: true
			color: Qt.rgba(78/255, 91/255, 242/255, control.pressed ? 0.55 : 0.35)
			samples: control.pressed ? 18 : 12
			horizontalOffset: 0
			verticalOffset: 0
		}

		// Value bubble
		Item {
			id: bubbleRoot
			visible: control.showValueBubble && (control.hovered || control.pressed)
			anchors.horizontalCenter: parent.horizontalCenter
			y: -height - 8
			width: valueText.implicitWidth + 12
			height: valueText.implicitHeight + 8
			opacity: visible ? 1 : 0

			Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

			Rectangle {
				id: bubble
				anchors.centerIn: parent
				width: parent.width
				height: parent.height
				radius: 6
				color: "#111827"
				border.color: "#1F2937"
				border.width: 1

				Text {
					id: valueText
					anchors.centerIn: parent
					text: control.decimals > 0 ? Number(control.value).toFixed(control.decimals) + (control.unitSuffix.length ? control.unitSuffix : "") : Math.round(control.value).toString() + (control.unitSuffix.length ? control.unitSuffix : "")
					color: control.textColor
					font.pixelSize: 12 * (Globals.maximised ? control.fontScaleFactor : 1)
					font.weight: Font.Thin
				}
			}

			// little pointer under bubble
			Canvas {
				id: pointer
				anchors.top: bubble.bottom
				anchors.horizontalCenter: bubble.horizontalCenter
				width: 10
				height: 6
				contextType: "2d"
				onPaint: {
					var ctx = getContext("2d");
					ctx.reset();
					ctx.beginPath();
					ctx.moveTo(0, 0);
					ctx.lineTo(width, 0);
					ctx.lineTo(width/2, height);
					ctx.closePath();
					ctx.fillStyle = "#111827";
					ctx.fill();
					ctx.lineWidth = 1;
					ctx.strokeStyle = "#1F2937";
					ctx.stroke();
				}
			}
		}
	}
}


