import QtQuick
import QtQuick.Controls
import ".."
import QtQuick.Layouts

Menu {
	id: appMenu
	// Appearance
	modal: false
	closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
	padding: 0
	spacing: 2
	transformOrigin: Item.TopLeft
	
	// Theme hooks (match app)
	property color backgroundColor: "#0F172A"
	property color borderColor: "#4E5BF2"
	property color highlightColor: "#144E5BF2"
	property color pressColor: "#224E5BF2"
	property color textColor: "#E5E7EB"
	property color mutedTextColor: "#9ca3af"
	property int cornerRadius: 12
	property int itemRadius: 8
	property int minWidth: 220
	property int maxHeight: Math.round(Screen.desktopAvailableHeight * 0.6)
	
	enter: Transition {
		NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 140; easing.type: Easing.OutCubic }
		NumberAnimation { properties: "scale"; from: 0.98; to: 1.0; duration: 140; easing.type: Easing.OutCubic }
	}
	
	exit: Transition {
		NumberAnimation { properties: "opacity"; from: 1; to: 0; duration: 100; easing.type: Easing.InCubic }
	}

	// Tight paddings to avoid top/bottom gaps
	topPadding: 0
	bottomPadding: 0

	background: Rectangle {
		id: bg
		implicitWidth: Math.max(appMenu.contentItem.implicitWidth + appMenu.padding * 2, appMenu.minWidth)
		implicitHeight: Math.min(appMenu.contentItem.implicitHeight + appMenu.padding * 2, appMenu.maxHeight)
		radius: appMenu.cornerRadius
		border.color: appMenu.borderColor
		border.width: 1
		color: appMenu.backgroundColor
		clip: true
	}

	// Style menu items per customization guide structure
	delegate: MenuItem {
		id: menuItem
		implicitWidth: Math.max(appMenu.minWidth, 200)
		implicitHeight: 36

		// Right arrow for submenu
		arrow: Canvas {
			x: parent.width - width
			implicitWidth: 40
			implicitHeight: 36
			visible: !!menuItem.subMenu
			onPaint: {
				var ctx = getContext("2d")
				ctx.reset()
				ctx.fillStyle = menuItem.highlighted ? appMenu.textColor : appMenu.borderColor
				ctx.moveTo(15, 10)
				ctx.lineTo(width - 15, height / 2)
				ctx.lineTo(15, height - 10)
				ctx.closePath()
				ctx.fill()
			}
		}

		// Left indicator for checkable items
		indicator: Item {
			implicitWidth: 40
			implicitHeight: 36
			Rectangle {
				width: 18
				height: 18
				anchors.centerIn: parent
				visible: menuItem.checkable
				border.color: appMenu.borderColor
				radius: 4
				Rectangle {
					width: 10
					height: 10
					anchors.centerIn: parent
					visible: menuItem.checked
					color: appMenu.borderColor
					radius: 3
				}
			}
		}

		contentItem: Text {
			leftPadding: menuItem.checkable ? 40 : 12
			rightPadding: (!!menuItem.menu) ? 40 : 12
			text: menuItem.text
			font: menuItem.font
			opacity: menuItem.enabled ? 1.0 : 0.4
			color: appMenu.textColor
			horizontalAlignment: Text.AlignLeft
			verticalAlignment: Text.AlignVCenter
			elide: Text.ElideRight
		}

		background: Rectangle {
			implicitWidth: Math.max(appMenu.minWidth, 200)
			implicitHeight: 36
			opacity: menuItem.enabled ? 1 : 0.3
			color: menuItem.down ? appMenu.pressColor : (menuItem.highlighted ? appMenu.highlightColor : "transparent")
			radius: appMenu.itemRadius
			border.width: menuItem.highlighted ? 1 : 0
			border.color: menuItem.highlighted ? Qt.lighter(appMenu.borderColor, 1.08) : "transparent"
		}
	}

	
}


