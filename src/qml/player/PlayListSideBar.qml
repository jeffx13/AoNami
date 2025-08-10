import QtQuick
import "./../components"
import QtQuick.Controls 2.15
import Kyokou.App.Main
import QtQuick.Layouts 1.15
Rectangle{
    id: sideBar
    property alias treeView: treeView

    color: '#d0303030'

    Connections {
        target: App.play
        function onCurrentIndexChanged() {
            treeView.forceLayout()
            sideBar.scrollToIndex(App.play.currentModelIndex)
            selection.clear()
            selection.setCurrentIndex(App.play.currentModelIndex, ItemSelectionModel.Select)
            selection.setCurrentIndex(App.play.currentListIndex, ItemSelectionModel.Select)
        }
    }

    onVisibleChanged: if (visible) {
                          sideBar.scrollToIndex(App.play.currentModelIndex)
                      }

    function scrollToIndex(index){
        if (index.valid) {
            treeView.collapseRecursively()
            treeView.expandToIndex(index);
            treeView.positionViewAtIndex(index, TableView.AlignVCenter)
            treeView.forceLayout()
        }
    }
    // https://forum.qt.io/topic/160590/qml-treeview-with-custom-delegate
    TreeView {
        id: treeView
        model: App.play
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
            id: selection
            model: App.play
        }

        delegate: TreeViewDelegate {
            id:treeDelegate
            implicitWidth: sideBar.width
            indentation: 20
            padding: 5
            required property bool isCurrentIndex
            required property string numberTitle

            property real fontSize: hasChildren ? 22 * root.fontSizeMultiplier : 20 * root.fontSizeMultiplier


            // onYChanged: {
            //     if(current)
            //         treeDelegate.treeView.contentY = treeDelegate.y;
            // }

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
                        let i = App.play.getCurrentIndex(index)
                        treeView.positionViewAtIndex(i, TableView.AlignVCenter)

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
                x: padding + (depth * indentation) + 5
                anchors.verticalCenter: parent.verticalCenter
                text: "▶"
                rotation: treeDelegate.expanded ? 90 : 0
                color: "deepskyblue"
                font.bold: true
                font.pixelSize: treeDelegate.fontSize
            }

            contentItem: Text {
                id: playlistItemLabel
                x: (isTreeNode ? (depth + 1) * indentation : 0)
                font.pixelSize: treeDelegate.fontSize
                maximumLineCount: 2
                clip: true
                text: treeDelegate.numberTitle
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                color: treeDelegate.selected ? "red"
                                             : treeDelegate.isCurrentIndex ? "green"
                                                                           : "white"

                Text {
                    id: deleteButton
                    visible: treeDelegate.hasChildren && !treeDelegate.selected
                    anchors{
                        right: parent.right
                        top: parent.top
                        bottom: parent.bottom
                    }
                    text: "✘"
                    font.pixelSize: parent.font.pixelSize
                    color: "white"
                    verticalAlignment: Text.AlignVCenter
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (treeDelegate.selected) return;
                            App.play.removeByModelIndex(index)
                        }
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
                    sideBar.scrollToIndex(App.play.currentModelIndex)
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
