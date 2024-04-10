import QtQuick
import "./../components"
import QtQuick.Controls 2.15

Rectangle{
    id:playlistBar
    property alias treeView: treeView

    color: '#d0303030'

    Connections {
        target: app.playlist
        function onCurrentIndexChanged() {
            treeView.forceLayout()
            playlistBar.scrollToIndex(app.playlist.currentIndex)
            selection.clear()
            selection.setCurrentIndex(app.playlist.currentIndex, ItemSelectionModel.Select)
            selection.setCurrentIndex(app.playlist.currentListIndex, ItemSelectionModel.Select)
        }
    }

    onVisibleChanged: if (visible) {
                          playlistBar.scrollToIndex(app.playlist.currentIndex)
                      }

    function scrollToIndex(index){
        if (index.valid) {
            treeView.collapseRecursively()
            treeView.expandToIndex(index);
            treeView.positionViewAtIndex(index, TableView.AlignVCenter)
            treeView.forceLayout()
        }
    }

    TreeView {
        id: treeView
        model: app.playlist
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

        selectionBehavior:TableView.SelectRows

        selectionModel: ItemSelectionModel {
            id:selection
            model: app.playlist
        }

        delegate: TreeViewDelegate {
            id:treeDelegate
            implicitWidth :treeView.width
            onYChanged: {
                if(current)
                    treeDelegate.treeView.contentY = treeDelegate.y;
            }

            TapHandler {
                acceptedModifiers: Qt.NoModifier
                onTapped: {
                    if (!treeDelegate.hasChildren) {
                        root.mpv.pause()
                        app.playlist.loadIndex(index)
                        return;
                    }

                    if (treeView.isExpanded(row)) {
                        treeView.collapse(row)
                        treeView.forceLayout()
                    } else {
                        treeView.expand(row)
                    }

                }
            }

            background: Rectangle {
                anchors.fill: parent
                color: "#d0303030"
            }

            indicator: Text {
                id: indicator
                visible: isTreeNode && hasChildren
                x: padding + (treeDelegate.depth * treeDelegate.indent)
                anchors.verticalCenter: treeDelegate.verticalCenter
                text: "▸"
                rotation: treeDelegate.expanded ? 90 : 0
                color: "deepskyblue"
                font.bold: true
                font.pixelSize: 20 * root.fontSizeMultiplier
                height: font.pixelSize
            }

            contentItem: Text {
                id: label
                x: padding + (treeDelegate.isTreeNode ? (treeDelegate.depth + 1) * treeDelegate.indent : 0)
                width: treeDelegate.width - treeDelegate.padding - x
                font.pixelSize: treeDelegate.hasChildren ? 22 * root.fontSizeMultiplier : 18 * root.fontSizeMultiplier
                height: treeDelegate.hasChildren ? font.pixelSize : font.pixelSize * 2
                clip: true
                text: model.numberTitle
                elide: Text.ElideRight
                color: selected ? "red" : "white"
            }
        }

    }

    Rectangle {
        id:bottomBar
        anchors{
            bottom:parent.bottom
            left: parent.left
            right: parent.right
        }
        color: "#3C4144"
        height: 40
        CustomButton {
            id: findCurrentIndexButton
            text: qsTr("Find current")
            anchors{
                top:parent.top
                left: parent.left
                bottom: parent.bottom
            }
            onClicked: {
                playlistBar.scrollToIndex(app.playlist.currentIndex)
            }

            fontSize: 20
        }
        CustomButton {
            text: qsTr("Collapse all")
            anchors{
                top:parent.top
                left: findCurrentIndexButton.right
                bottom: parent.bottom
            }
            onClicked: {
                treeView.collapseRecursively()
                treeView.contentY = 0
            }

            fontSize: 20
        }
    }
}
