import QtQuick
import QtQuick.Controls
import ".."

Menu {
	id: appMenu
	modal: false
	closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
	padding: 0
	spacing: 2
	transformOrigin: Item.TopLeft
	
	readonly property int minWidth: 220
	readonly property int maxHeight: Math.round(Screen.desktopAvailableHeight * 0.6)
	
	enter: Transition {
		NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 140; easing.type: Easing.OutCubic }
		NumberAnimation { properties: "scale"; from: 0.98; to: 1.0; duration: 140; easing.type: Easing.OutCubic }
	}
	
	exit: Transition {
		NumberAnimation { properties: "opacity"; from: 1; to: 0; duration: 100; easing.type: Easing.InCubic }
	}

	topPadding: 0
	bottomPadding: 0

	background: Rectangle {
		implicitWidth: Math.max(appMenu.contentItem.implicitWidth + appMenu.padding * 2, appMenu.minWidth)
		implicitHeight: Math.min(appMenu.contentItem.implicitHeight + appMenu.padding * 2, appMenu.maxHeight)
		radius: 12
		border.color: Theme.accent
		border.width: 1
		color: Theme.surface
		clip: true
	}

	delegate: MenuItem {
		id: menuItem
		implicitWidth: Math.max(appMenu.minWidth, menuLabel.implicitWidth)   // minWidth is a floor
		implicitHeight: 36

		arrow: Canvas {
			x: parent.width - width
			implicitWidth: 40
			implicitHeight: 36
			visible: !!menuItem.subMenu
			onPaint: {
				var ctx = getContext("2d")
				ctx.reset()
				ctx.fillStyle = menuItem.highlighted ? Theme.textPrimary : Theme.accent
				ctx.moveTo(15, 10)
				ctx.lineTo(width - 15, height / 2)
				ctx.lineTo(15, height - 10)
				ctx.closePath()
				ctx.fill()
			}
		}

		indicator: Item {
			implicitWidth: 40
			implicitHeight: 36
			Rectangle {
				width: 18
				height: 18
				anchors.centerIn: parent
				visible: menuItem.checkable
				border.color: Theme.accent
				radius: 4
				Rectangle {
					width: 10
					height: 10
					anchors.centerIn: parent
					visible: menuItem.checked
					color: Theme.accent
					radius: 3
				}
			}
		}

		contentItem: Text {
			id: menuLabel
			leftPadding: menuItem.checkable ? 40 : 12
			rightPadding: (!!menuItem.subMenu) ? 40 : 12
			text: menuItem.text
			font: menuItem.font
			opacity: menuItem.enabled ? 1.0 : 0.4
			color: Theme.textPrimary
			horizontalAlignment: Text.AlignLeft
			verticalAlignment: Text.AlignVCenter
			elide: Text.ElideRight
		}

		background: Rectangle {
			implicitWidth: menuItem.implicitWidth
			implicitHeight: 36
			opacity: menuItem.enabled ? 1 : 0.3
			color: menuItem.down      ? Qt.alpha(Theme.accent, 0.28)
			     : menuItem.highlighted ? Qt.alpha(Theme.accent, 0.16) : "transparent"
			radius: 8
			border.width: menuItem.highlighted ? 1 : 0
			border.color: menuItem.highlighted ? Qt.lighter(Theme.accent, 1.08) : "transparent"
		}
	}

	
}


