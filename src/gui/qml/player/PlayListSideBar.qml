import QtQuick
import "./../components"
import QtQuick.Controls 2.15
import Kyokou.App.Main
import QtQuick.Layouts 1.15
Rectangle{
    id: sideBar
    property alias treeView: treeView

    color: "#0E162B"

    Connections {
        target: App.playlistModel

        function onSelectionsChanged(index, scroll) {
            selection.clear()
            treeView.currentIndex = index
            let cur = index
            while (cur.valid) {
                selection.select(cur, ItemSelectionModel.Select)
                cur = cur.parent
            }
            if (scroll) scrollToIndex(index);


        }
    }

    function scrollToIndex(index){
        if (index === undefined || !index.valid) return;
        treeView.collapseRecursively()
        treeView.contentY = 0
        treeView.expandToIndex(index);
        treeView.forceLayout()
        treeView.positionViewAtIndex(index, TableView.AlignVCenter)

    }

    // https://forum.qt.io/topic/160590/qml-treeview-with-custom-delegate
    TreeView {
        id: treeView
        model: App.playlistModel
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        boundsMovement: Flickable.StopAtBounds
        keyNavigationEnabled:false
        smooth: false
        pointerNavigationEnabled: false
        anchors {
            top: parent.top
            right: parent.right
            left: parent.left
            bottom: bottomBar.top
        }
        property var currentIndex: undefined

        selectionBehavior:TableView.SelectRows

        selectionModel: ItemSelectionModel {
            id: selection
            model: App.playlistModel
        }

        delegate: Item {
            id:delegate
            implicitWidth: sideBar.width
            implicitHeight: label.implicitHeight * (hasChildren ? 1.5 : 1.75)

            readonly property real indentation: 20
            readonly property real padding: 5
            property real fontSize: root.fontSizeMultiplier * (hasChildren ? 22 : 20)

            required property bool isCurrentIndex
            required property string numberTitle
            required property bool isDeletable

            required property TreeView treeView
            required property bool isTreeNode
            required property bool expanded
            required property int hasChildren
            required property int depth
            required property int row
            required property int column
            required property bool current
            required property bool selected
            required property var index

            TapHandler {
                acceptedModifiers: Qt.NoModifier
                onTapped: {
                    if (!hasChildren) {
                        mpvPlayer.pause()
                        App.play.loadIndex(index)
                        return;
                    }
                    if (treeView.isExpanded(row)) {
                        treeView.collapse(row)
                    } else {
                        treeView.expand(row)
                        treeView.forceLayout()
                        let i = App.playlistModel.getCurrentIndex(delegate.index)
                        treeView.positionViewAtIndex(i, TableView.AlignVCenter)

                    }

                }
            }

            Rectangle {
                anchors.fill: parent
                color: "#0E162B"
                border.width: 1
                border.color: hasChildren ? "#4E5BF2" : "#2B2F44"
            }

            Text {
                id: indicator
                visible: isTreeNode && hasChildren
                x: padding + (depth * indentation) + 5
                anchors.verticalCenter: parent.verticalCenter
                text: "▶"
                rotation: expanded ? 90 : 0
                color: "#4E5BF2"
                font.bold: true
                font.pixelSize: delegate.fontSize
            }

            Text {
                id: label
                font.pixelSize: delegate.fontSize
                x: (isTreeNode ? (depth + 1) * indentation : 0)
                width: delegate.width - deleteButton.width - 20
                maximumLineCount: 2
                clip: true
                text: delegate.numberTitle
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                color: selected ? "red" : (isCurrentIndex ? "green" : "white")
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: deleteButton
                visible: !delegate.selected && delegate.isDeletable // delegate.hasChildren &&
                text: "✘"
                font.pixelSize: delegate.fontSize
                color: "white"
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        App.play.remove(delegate.index)
                    }
                }
            }



        }

    }

    Rectangle {
        id:bottomBar
        anchors {
            bottom:parent.bottom
            left: parent.left
            right: parent.right
        }
        color: "#3C4144"
        height: 80
        GridLayout {
            anchors.fill: parent
            columns: 2
            rows: 2
            CustomButton {
                id: findCurrentIndexButton
                text: qsTr("Find current")
                Layout.row: 0
                Layout.column: 0
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredHeight: 5
                onClicked: {
                    sideBar.scrollToIndex(treeView.currentIndex)
                }
                fontSize: 20
            }
            CustomButton {
                text: qsTr("Close All")
                onClicked: App.play.clear()
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredHeight: 5
                Layout.row: 0
                Layout.column: 1
                fontSize: 20
            }

            CustomButton {
                text: qsTr("Collapse all")
                Layout.row: 1
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredHeight: 5

                onClicked: {
                    treeView.collapseRecursively()
                    treeView.contentY = 0
                }

                fontSize: 20
            }
        }


    }
}
