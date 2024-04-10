import QtQuick 2.0
import QtQuick.Controls 2.0
import Qt5Compat.GraphicalEffects

ComboBox {
    id: comboBox

    property color checkedColor: "#4E5BF2"
    property color currentIndexColor: "red"
    property int fontSize: 20
    readonly property int scaledFontSize: fontSize * (root.maximised ? fontScaleFactor : 1)
    property int fontScaleFactor: 2

    property string text: ""
    property int hAlignment: Text.AlignHCenter
    property int vAlignment: Text.AlignVCenter

    property int contentRadius: 5

    delegate: ItemDelegate {
        width: comboBox.width
        contentItem: Text {
            text: comboBox.text.length === 0 ? modelData : model[`${comboBox.text}`]
            color: comboBox.highlightedIndex === index ? "white" : "black"
            elide: Text.ElideRight
            font.pixelSize: scaledFontSize
            verticalAlignment: vAlignment
            horizontalAlignment: hAlignment
        }

        background: Rectangle {
            width: parent.width
            height: parent.height
            color: {
                if (index === comboBox.currentIndex) return currentIndexColor
                return comboBox.highlightedIndex === index ? comboBox.checkedColor : "#F3F4F5"
            }
            radius: 20
        }
    }

    indicator: Canvas {
        id: canvas
        x: comboBox.width - width - 10
        y: (comboBox.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: comboBox
            function onPressedChanged(){
                canvas.requestPaint()
            }
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = "white"
            context.fill();
        }
    }

    contentItem: Text {
        width: comboBox.background.width - comboBox.indicator.width - 10
        height: comboBox.background.height
        anchors.verticalCenter: parent.verticalCenter
        x: 10
        text: comboBox.displayText
        elide: Text.ElideRight
        horizontalAlignment: hAlignment
        verticalAlignment: vAlignment
        font.pixelSize: scaledFontSize
        font.weight: Font.Thin
        color: comboBox.down ? Qt.rgba(255, 255, 255, 0.75) : "white"
    }

    background: Rectangle {
        implicitWidth: 102
        implicitHeight: 41
        color: comboBox.down ? Qt.darker(comboBox.checkedColor, 1.2) : comboBox.checkedColor
        radius: height/2

        layer.enabled: comboBox.hovered | comboBox.down
        layer.effect: DropShadow {
            transparentBorder: true
            color: comboBox.checkedColor
            samples: 10 /*20*/
        }
    }

    popup: Popup {
        y: comboBox.height - 1
        width: comboBox.width
        implicitHeight: contentItem.implicitHeight
        padding: 0



        contentItem: ListView {
            implicitHeight: contentHeight
            model: comboBox.popup.visible ? comboBox.delegateModel : null
            clip: true
            currentIndex: comboBox.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }
        background: Rectangle {
            color: "#F3F4F5"
            radius: 20
            clip: true
            layer.enabled: comboBox.hovered | comboBox.down
        }
    }
}
