import QtQuick
import "./../Components"
import QtQuick.Controls
import AoNami
import QtQuick.Layouts
import ".."

Rectangle {
    id: sideBar
    property alias treeView: treeView
    property string showName: ""
    property int    epCount: 0
    property string filterText: ""
    signal hideRequested()
    color: Theme.surfaceDeep

    // Re-layout so filtered (0-height) rows actually collapse instead of leaving gaps.
    onFilterTextChanged: treeView.forceLayout()

    // Hide tab - a pill in the middle of the left border that collapses the sidebar.
    Rectangle {
        id: hideTab
        width: 16
        height: 56
        radius: 7
        anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 0 }
        color: hideTabArea.containsMouse ? Theme.accent : "#11192C"
        border.color: hideTabArea.containsMouse ? Theme.accent : Theme.border
        border.width: 1
        z: 20
        Behavior on color { ColorAnimation { duration: 120 } }
        Text {
            anchors.centerIn: parent
            text: "›"
            color: hideTabArea.containsMouse ? "white" : Theme.textSecondary
            font.pixelSize: Globals.sp(28)
            font.bold: true
        }
        MouseArea {
            id: hideTabArea
            anchors.fill: parent
            anchors.margins: -3
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: sideBar.hideRequested()
        }
    }

    component HeaderIcon: Rectangle {
        id: hicon
        property string kind: ""
        property string tip: ""
        signal activated()
        width: 32; height: 32; radius: 8
        readonly property color ic: hoverMa.containsMouse ? Theme.textPrimary : Theme.textSecondary
        color: hoverMa.containsMouse ? Theme.border : "transparent"
        Behavior on color { ColorAnimation { duration: 120 } }

        Item {                                  // locate: target ring + dot
            anchors.centerIn: parent
            visible: hicon.kind === "locate"
            width: 15; height: 15
            Rectangle { anchors.fill: parent; radius: width / 2; color: "transparent"; border.color: hicon.ic; border.width: 2 }
            Rectangle { anchors.centerIn: parent; width: 5; height: 5; radius: 2.5; color: hicon.ic }
        }
        Column {                                // collapse: stacked bars
            anchors.centerIn: parent
            visible: hicon.kind === "collapse"
            spacing: 3
            Rectangle { width: 15; height: 2.4; radius: 1.2; color: hicon.ic }
            Rectangle { width: 15; height: 2.4; radius: 1.2; color: hicon.ic }
        }
        Item {                                  // clear: X
            anchors.centerIn: parent
            visible: hicon.kind === "clear"
            width: 15; height: 15
            Rectangle { anchors.centerIn: parent; width: 17; height: 2.4; radius: 1.2; color: hicon.ic; rotation: 45 }
            Rectangle { anchors.centerIn: parent; width: 17; height: 2.4; radius: 1.2; color: hicon.ic; rotation: -45 }
        }

        AppToolTip { text: hicon.tip; visible: hoverMa.containsMouse }
        MouseArea {
            id: hoverMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: hicon.activated()
        }
    }

    Rectangle {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 46
        color: "#0D1220"

        RotatingText {
            anchors {
                left: parent.left; leftMargin: 14
                right: countBadge.visible ? countBadge.left : headerBtns.left
                rightMargin: 8
                verticalCenter: parent.verticalCenter
            }
            text: sideBar.showName.length > 0 ? sideBar.showName : "Playlist"
            fontSize: 21
            color: Theme.textPrimary
            horizontalAlignment: Text.AlignLeft
        }

        Rectangle {
            id: countBadge
            visible: sideBar.epCount > 0
            anchors { right: headerBtns.left; rightMargin: 8; verticalCenter: parent.verticalCenter }
            width: countText.implicitWidth + 14
            height: 20
            radius: 10
            color: "#1F4E5BF2"
            border.color: "#3D4E5BF2"
            border.width: 1
            Text {
                id: countText
                anchors.centerIn: parent
                text: sideBar.epCount + (sideBar.epCount === 1 ? " ep" : " eps")
                color: Theme.textAccent
                font.pixelSize: Globals.sp(15)
                font.weight: Font.Medium
            }
        }

        Row {
            id: headerBtns
            anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
            spacing: 2
            HeaderIcon { kind: "locate";   tip: "Scroll to current"; onActivated: sideBar.scrollToIndex(treeView.currentIndex) }
            HeaderIcon { kind: "collapse"; tip: "Collapse all";      onActivated: { treeView.collapseRecursively(); treeView.forceLayout(); treeView.contentY = 0 } }
            HeaderIcon { kind: "clear";    tip: "Close all";         onActivated: App.play.clear() }
        }

        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: 1
            color: "#10ffffff"
        }
    }

    Connections {
        target: App.playlistModel
        function onCurrentItemChanged(index) {
            selection.clear()
            treeView.currentIndex = index
            let cur = index
            while (cur.valid) {
                selection.select(cur, ItemSelectionModel.Select)
                cur = cur.parent
            }
            treeView.collapseRecursively()
            scrollToIndex(index)
            sideBar.showName = App.playlistModel.currentShowName()
            sideBar.epCount = App.playlistModel.currentShowEpisodeCount()
        }
    }

    function scrollToIndex(index) {
        if (index === undefined || !index.valid) return
        treeView.contentY = 0
        treeView.expandToIndex(index)
        treeView.forceLayout()
        treeView.positionViewAtIndex(index, TableView.AlignVCenter)
    }

    Rectangle {
        id: filterRow
        anchors { top: header.bottom; left: parent.left; right: parent.right }
        height: 42
        color: Theme.surfaceDeep

        AppTextField {
            id: filterField
            anchors { fill: parent; leftMargin: 8; rightMargin: 8; topMargin: 5; bottomMargin: 5 }
            placeholderText: "Filter episodes..."
            color: "white"
            placeholderTextColor: "#4B5563"
            fontSize: 19
            onTextChanged: sideBar.filterText = text
        }
    }

    TreeView {
        id: treeView
        model: App.playlistModel
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        boundsMovement: Flickable.StopAtBounds
        keyNavigationEnabled: false
        smooth: false
        pointerNavigationEnabled: false
        selectionBehavior: TableView.SelectRows
        property var currentIndex: undefined

        // Collapse filtered-out rows to 0 height (delegate implicitHeight isn't
        // re-measured when hidden); negative falls back to the implicit height.
        rowHeightProvider: function(row) {
            if (sideBar.filterText.length === 0) return -1
            return App.playlistModel.isFilteredOut(treeView.index(row, 0), sideBar.filterText) ? 0 : -1
        }

        anchors {
            top: filterRow.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
            topMargin: 2
        }

        ScrollBar.vertical: ScrollBar {
            id: sbScroll
            policy: ScrollBar.AsNeeded
            width: 9
            minimumSize: 0.06
            anchors { top: treeView.top; right: treeView.right; bottom: treeView.bottom; rightMargin: 1 }
            contentItem: Rectangle {
                radius: 4
                color: Theme.accent
                opacity: sbScroll.pressed ? 0.9 : (sbScroll.hovered ? 0.7 : 0.45)
                Behavior on opacity { NumberAnimation { duration: 120 } }
            }
            background: Rectangle { radius: 4; color: "#10ffffff" }
        }

        selectionModel: ItemSelectionModel {
            id: selection
            model: App.playlistModel
        }

        delegate: Item {
            id: del
            implicitWidth: sideBar.width
            implicitHeight: card.height + 4
            clip: true
            visible: !filteredOut   // hide content in rows the provider collapsed to 0

            readonly property real indent: 20
            // Filter hides non-matching episode leaves (matches display name or number).
            readonly property bool filteredOut: sideBar.filterText.length > 0 && !del.hasChildren
                && del.display.toLowerCase().indexOf(sideBar.filterText.toLowerCase()) === -1
                && String(del.number).indexOf(sideBar.filterText) === -1

            required property bool isCurrentIndex
            required property string display
            required property bool isDeletable
            required property string link
            required property bool isWatched
            required property real number
            required property TreeView treeView
            required property bool isTreeNode
            required property bool expanded
            required property bool hasChildren
            required property int depth
            required property int row
            required property int column
            required property bool current
            required property bool selected
            required property var index

            TapHandler {
                acceptedButtons: Qt.LeftButton
                acceptedModifiers: Qt.NoModifier
                onTapped: {
                    if (!del.hasChildren) {
                        mpv.pause()
                        App.play.loadIndex(del.index)
                        return
                    }
                    if (del.treeView.isExpanded(del.row))
                        del.treeView.collapse(del.row)
                    else {
                        del.treeView.expand(del.row)
                        sideBar.scrollToIndex(App.playlistModel.getCurrentIndex(del.index))
                    }
                }
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: ctxMenu.popup()
            }

            AppMenu {
                id: ctxMenu
                modal: true
                Action { text: "Play"; enabled: !del.hasChildren; onTriggered: { mpv.pause(); App.play.loadIndex(del.index) } }
                Action { text: "Copy link"; enabled: del.link.length > 0; onTriggered: { App.copyToClipboard(del.link); mpv.showText("Copied link") } }
                Action { text: "Remove"; enabled: del.isDeletable; onTriggered: App.play.remove(del.index) }
            }

            Rectangle {
                id: card
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: 4
                    rightMargin: 4
                }
                height: Math.max(del.hasChildren ? 34 : 40, itemText.height + 16)
                radius: 6
                color: del.selected ? "#15ffffff" : (cardHover.hovered ? "#0Affffff" : "transparent")
                border.color: del.hasChildren ? Theme.border : "transparent"
                border.width: del.hasChildren ? 1 : 0

                HoverHandler { id: cardHover }

                Rectangle {
                    visible: del.selected && !del.hasChildren
                    width: 3
                    radius: 1.5
                    color: Theme.accent
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                        leftMargin: 1
                        topMargin: 8
                        bottomMargin: 8
                    }
                }

                Text {
                    id: arrow
                    visible: del.isTreeNode && del.hasChildren
                    x: 8 + del.depth * del.indent
                    anchors.verticalCenter: parent.verticalCenter
                    text: "\u25B8"
                    rotation: del.expanded ? 90 : 0
                    color: Theme.accent
                    font {
                        pixelSize: Globals.sp(20)
                        bold: true
                    }
                    Behavior on rotation { NumberAnimation { duration: 120 } }
                }

                Text {
                    id: itemText
                    x: del.isTreeNode ? (del.depth + 1) * del.indent + 4 : 8
                    width: card.width - x - 38
                    anchors.verticalCenter: parent.verticalCenter
                    maximumLineCount: 2
                    text: del.display
                    elide: Text.ElideRight
                    wrapMode: Text.WordWrap
                    font.pixelSize: Globals.sp(20)
                    color: del.selected       ? Theme.textAccent
                         : del.isCurrentIndex ? Theme.success
                         : del.isWatched      ? "#5A6577"
                         : Theme.textSecondary
                }

                Text {                                  // watched check
                    visible: del.isWatched && !del.hasChildren && !del.isDeletable
                             && !del.isCurrentIndex && !cardHover.hovered
                    text: "\u2713"
                    color: "#5A6577"
                    font.pixelSize: Globals.sp(18)
                    anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                }

                Item {                                  // hover copy-link (leaf, non-deletable)
                    id: copyBtn
                    visible: cardHover.hovered && !del.hasChildren && !del.isDeletable && del.link.length > 0
                    width: 26; height: 26
                    anchors { right: parent.right; rightMargin: 6; verticalCenter: parent.verticalCenter }
                    Rectangle {
                        x: 9; y: 4; width: 11; height: 13; radius: 3
                        color: "transparent"
                        border.color: copyArea.containsMouse ? Theme.accent : Theme.textMuted
                        border.width: 1.5
                    }
                    Rectangle {
                        x: 5; y: 8; width: 11; height: 13; radius: 3
                        color: Theme.surfaceDeep
                        border.color: copyArea.containsMouse ? Theme.accent : Theme.textMuted
                        border.width: 1.5
                    }
                    MouseArea {
                        id: copyArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { App.copyToClipboard(del.link); mpv.showText("Copied link") }
                    }
                }

                Text {
                    id: delBtn
                    visible: !del.selected && del.isDeletable
                    text: "\u2715"
                    font.pixelSize: Globals.sp(20)
                    color: delBtnArea.containsMouse ? Theme.danger : "#374151"
                    anchors {
                        right: parent.right
                        rightMargin: 8
                        verticalCenter: parent.verticalCenter
                    }

                    MouseArea {
                        id: delBtnArea
                        anchors.fill: parent
                        anchors.margins: -6
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.play.remove(del.index)
                    }
                }
            }
        }
    }

}