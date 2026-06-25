import QtQuick
import QtQuick.Controls
import ".."

ComboBox {
    id: comboBox

    property color checkedColor: Theme.accent
    property color surfaceColor: Theme.surface
    property color borderColor: Theme.border
    property color textColor: Theme.textPrimary
    property color currentIndexColor: Theme.surfaceAlt
    property int fontSize: 20

    property string text: "text"
    property int hAlignment: Text.AlignHCenter
    property int vAlignment: Text.AlignVCenter
    property string placeholderText: ""

    textRole: comboBox.text && comboBox.text.length > 0 ? comboBox.text : ""

    delegate: ItemDelegate {
        id: itemDel
        width: comboBox.width
        height: 40
        enabled: !model.disabled
        opacity: enabled ? 1.0 : 0.5

        readonly property bool isCurrent: index === comboBox.currentIndex

        contentItem: Text {
            leftPadding: 10
            rightPadding: 22
            text: (comboBox.text.length === 0 || typeof model[comboBox.text] === "undefined")
                  ? modelData : model[comboBox.text]
            color: comboBox.highlightedIndex === index ? "white"
                 : itemDel.isCurrent ? Theme.textAccent
                 : comboBox.textColor
            font.weight: itemDel.isCurrent ? Font.Medium : Font.Normal
            elide: Text.ElideRight
            font.pixelSize: Globals.sp(comboBox.fontSize)
            verticalAlignment: comboBox.vAlignment
            horizontalAlignment: comboBox.hAlignment
        }

        background: Rectangle {
            color: {
                if (itemDel.isCurrent) return comboBox.currentIndexColor
                return comboBox.highlightedIndex === index ? comboBox.checkedColor : comboBox.surfaceColor
            }
            radius: 8

            Rectangle {                                  // accent bar on the selected item
                visible: itemDel.isCurrent
                anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 2 }
                width: 3; height: parent.height - 14; radius: 1.5
                color: Theme.accent
            }
            AppIcon {                                    // checkmark on the selected item
                visible: itemDel.isCurrent
                anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 9 }
                name: "check"
                size: 16
                color: comboBox.highlightedIndex === index ? "white" : Theme.accent
            }
        }
    }

    indicator: AppIcon {
        x: comboBox.width - width - 12
        anchors.verticalCenter: parent.verticalCenter
        name: "chevron-down"
        size: 16
        color: comboBox.down ? Theme.textAccent : Theme.textMuted
    }

    contentItem: Text {
        width: comboBox.background.width - 30
        height: comboBox.background.height
        anchors.verticalCenter: parent.verticalCenter
        x: 12
        text: comboBox.currentIndex < 0 && comboBox.placeholderText.length > 0
              ? comboBox.placeholderText : comboBox.displayText
        elide: Text.ElideRight
        horizontalAlignment: comboBox.hAlignment
        verticalAlignment: comboBox.vAlignment
        font.pixelSize: Globals.sp(comboBox.fontSize)
        color: comboBox.currentIndex < 0 && comboBox.placeholderText.length > 0
               ? Theme.textMuted : comboBox.textColor
    }

    background: Rectangle {
        implicitWidth: 102
        implicitHeight: 41
        color: comboBox.surfaceColor
        radius: 12
        border.color: comboBox.activeFocus ? comboBox.checkedColor
                    : comboBox.hovered     ? Qt.lighter(comboBox.checkedColor, 1.3)
                    : comboBox.borderColor
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: 140 } }

        Rectangle {
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
                bottomMargin: -1
            }
            height: 2
            radius: 1
            width: comboBox.activeFocus ? parent.width - 20 : 0
            color: comboBox.checkedColor
            opacity: comboBox.activeFocus ? 1.0 : 0.0
            Behavior on width {
                NumberAnimation {
                    duration: 250
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
    }

    popup: Popup {
        parent: comboBox
        x: 0
        y: comboBox.height + 4
        width: comboBox.width
        implicitHeight: contentItem.implicitHeight + 8
        padding: 4
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        enter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 150
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "scale"
                    from: 0.95
                    to: 1.0
                    duration: 150
                    easing.type: Easing.OutCubic
                }
            }
        }
        exit: Transition {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 100
            }
        }

        contentItem: ListView {
            implicitHeight: contentHeight
            model: comboBox.popup.visible ? comboBox.delegateModel : null
            clip: true
            currentIndex: comboBox.highlightedIndex
            spacing: 2
            ScrollIndicator.vertical: ScrollIndicator {}
        }

        background: Rectangle {
            color: comboBox.surfaceColor
            border.color: Theme.border
            border.width: 1
            radius: 12

            Rectangle {
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    topMargin: 1
                    leftMargin: 10
                    rightMargin: 10
                }
                height: 1
                color: Theme.border
            }
        }
    }
}