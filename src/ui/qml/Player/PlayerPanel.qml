import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import AoNami
import ".."

Popup {
    id: panel
    visible: false
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var listModels: [App.playlist.serverList, Globals.mpv.videoList, Globals.mpv.audioList, Globals.mpv.subtitleList]

    readonly property var allTabs: [
        { id: "servers", label: "Servers", type: "list", modelIndex: 0 },
        { id: "video",   label: "Video",   type: "list", modelIndex: 1 },
        { id: "audio",   label: "Audio",   type: "list", modelIndex: 2 },
        { id: "subs",    label: "Subs",    type: "list", modelIndex: 3 },
        { id: "general", label: "General", type: "page" },
        { id: "skip",    label: "Skip",    type: "page" }
    ]

    readonly property var visibleTabs: {
        let tabs = []
        for (let i = 0; i < allTabs.length; i++) {
            let t = allTabs[i]
            if (t.type === "list") {
                let c = listModels[t.modelIndex].count
                // Subs stays with no tracks: that is exactly when you want to go looking.
                if (c === 0 && t.id !== "subs") continue
                if (t.id === "servers" && c <= 1) continue   // one server -> nothing to pick
            }
            tabs.push(t)
        }
        return tabs
    }

    property int activeTabIndex: 0
    property string activeTabId: "servers"
    component Chip: Rectangle {
        property alias label: chipLabel.text
        property bool filled: false
        implicitWidth: chipLabel.implicitWidth + 12
        implicitHeight: 20
        radius: 5
        color: filled ? Theme.accent : Qt.alpha(Theme.accent, 0.16)
        border.color: Qt.alpha(Theme.accent, 0.45)
        border.width: 1
        Text {
            id: chipLabel
            anchors.centerIn: parent
            color: parent.filled ? "#000000" : Theme.textAccent
            font.pixelSize: Globals.sp(13)
            font.bold: true
        }
    }

    property int subsSubPage: 0   // 0 = the player's tracks, 1 = SubDL search
    onSubsSubPageChanged: if (subsSubPage === 1) {
        subQueryField.offerShowName()
        App.subtitleSearch.searchIfNew(subQueryField.text)
    }
    readonly property var activeTab: (activeTabIndex >= 0 && activeTabIndex < visibleTabs.length)
                                     ? visibleTabs[activeTabIndex] : null

    function syncActiveTab() {
        let i = -1
        for (let k = 0; k < visibleTabs.length; k++)
            if (visibleTabs[k].id === activeTabId) { i = k; break }
        if (i < 0)
            for (let k = 0; k < visibleTabs.length; k++)
                if (visibleTabs[k].id === "video") { i = k; break }
        activeTabIndex = i >= 0 ? i : 0
    }
    onVisibleTabsChanged: syncActiveTab()

    background: Rectangle {
        radius: 18
        color: "#E8080D1A"
        border.color: "#18ffffff"
        border.width: 1

        Rectangle {
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                topMargin: 1
                leftMargin: 20
                rightMargin: 20
            }
            height: 1
            radius: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: "transparent"
                }
                GradientStop {
                    position: 0.5
                    color: "#504E5BF2"
                }
                GradientStop {
                    position: 1.0
                    color: "transparent"
                }
            }
        }

        Rectangle {
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                bottomMargin: 1
                leftMargin: 40
                rightMargin: 40
            }
            height: 1
            radius: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: "transparent"
                }
                GradientStop {
                    position: 0.5
                    color: "#204E5BF2"
                }
                GradientStop {
                    position: 1.0
                    color: "transparent"
                }
            }
        }
    }

    // Opacity-only - a scale animation re-rasterises the panel over the live video each frame.
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120; easing.type: Easing.InCubic }
    }

    onOpened: {
        syncActiveTab()
        Qt.callLater(() => {
            if (listView.model) listView.positionViewAtIndex(listView.currentIndex, ListView.Center)
            if (serverListView.model) serverListView.positionViewAtIndex(serverListView.currentIndex, ListView.Center)
        })
    }

    contentItem: ColumnLayout {
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 10

            Rectangle {
                anchors.fill: parent
                radius: 12
                color: "#08ffffff"

                Row {
                    id: tabRow
                    anchors {
                        fill: parent
                        margins: 3
                    }
                    spacing: 2

                    Repeater {
                        model: panel.visibleTabs.length
                        delegate: AbstractButton {
                            id: tab
                            required property int index
                            readonly property var tabData: panel.visibleTabs[tab.index]
                            readonly property bool isActive: panel.activeTabIndex === tab.index
                            readonly property bool isList: tab.tabData.type === "list"
                            readonly property int itemCount: tab.isList ? panel.listModels[tab.tabData.modelIndex].count : -1

                            width: (tabRow.width - (panel.visibleTabs.length - 1) * 2) / panel.visibleTabs.length
                            height: tabRow.height
                            focusPolicy: Qt.NoFocus
                            onClicked: { panel.activeTabIndex = tab.index; panel.activeTabId = panel.visibleTabs[tab.index].id }

                            background: Rectangle {
                                radius: 10
                                color: tab.isActive ? Theme.accent : (tab.hovered ? "#10ffffff" : "transparent")
                                Behavior on color { ColorAnimation { duration: 120 } }

                                Rectangle {
                                    visible: tab.isActive
                                    anchors {
                                        top: parent.top
                                        left: parent.left
                                        right: parent.right
                                        topMargin: 1
                                        leftMargin: 8
                                        rightMargin: 8
                                    }
                                    height: 1
                                    radius: 1
                                    color: "#40ffffff"
                                }
                            }

                            contentItem: Item {
                                Row {
                                    anchors.centerIn: parent
                                    spacing: 5
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: tab.tabData.label
                                        font.pixelSize: Globals.sp(18)
                                        font.bold: tab.isActive
                                        color: tab.isActive ? "white" : (tab.hovered ? "#9CA3AF" : "#4B5563")
                                        elide: Text.ElideRight
                                        width: Math.min(implicitWidth, tab.width - 14 - (tabCount.visible ? tabCount.width + 5 : 0))
                                        Behavior on color { ColorAnimation { duration: 120 } }
                                    }
                                    Rectangle {
                                        id: tabCount
                                        visible: tab.isList && tab.itemCount >= 0
                                        anchors.verticalCenter: parent.verticalCenter
                                        // Min width fits two digits, so 9 -> 10 doesn't elide the label.
                                        width: Math.max(20, tabCountText.implicitWidth + 8)
                                        height: 18
                                        radius: 9
                                        color: tab.isActive ? "#33ffffff" : "#10ffffff"
                                        Text {
                                            id: tabCountText
                                            anchors.centerIn: parent
                                            text: tab.itemCount
                                            font.pixelSize: Globals.sp(14)
                                            font.weight: Font.Medium
                                            color: tab.isActive ? "white" : "#6B7280"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                height: 3
                radius: 1.5
                color: Theme.accent
                y: parent.height - 1

                property real targetX: {
                    if (panel.activeTabIndex < 0 || panel.activeTabIndex >= tabRow.children.length)
                        return 0
                    let item = tabRow.children[panel.activeTabIndex]
                    if (!item) return 0
                    return item.x + tabRow.x + 3 + 12
                }
                property real targetW: {
                    if (panel.activeTabIndex < 0 || panel.activeTabIndex >= tabRow.children.length)
                        return 0
                    let item = tabRow.children[panel.activeTabIndex]
                    if (!item) return 0
                    return item.width - 24
                }

                x: targetX
                width: targetW

                Behavior on x {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }
                }
                Behavior on width {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }
                }

                Rectangle {
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: parent.top
                    }
                    width: parent.width * 0.6
                    height: 6
                    radius: 3
                    color: "#304E5BF2"
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 14
            Layout.rightMargin: 14
            Layout.topMargin: 10
            spacing: 8
            visible: panel.activeTab && panel.activeTab.id === "subs"

            Repeater {
                model: [qsTr("Tracks"), qsTr("Search")]
                delegate: Rectangle {
                    id: subTab
                    required property string modelData
                    required property int index
                    readonly property bool selected: panel.subsSubPage === index

                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    radius: 9
                    color: selected            ? Qt.alpha(Theme.accent, 0.22)
                         : subTabHover.hovered ? Qt.alpha(Theme.textPrimary, 0.07)
                                               : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }

                    Text {
                        anchors.centerIn: parent
                        text: subTab.modelData
                        color: subTab.selected ? Theme.accent : Theme.textSecondary
                        font.pixelSize: Globals.sp(18)
                        font.bold: subTab.selected
                    }

                    HoverHandler { id: subTabHover; cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: panel.subsSubPage = subTab.index }
                }
            }
        }

        // Without this a fetched subtitle can hold a slot with no row on either page to clear it.
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 14
            Layout.rightMargin: 14
            Layout.topMargin: 8
            spacing: 8
            visible: panel.activeTab && panel.activeTab.id === "subs"
                     && (Globals.mpv.primarySubId !== 0 || Globals.mpv.secondarySubId !== 0)

            Repeater {
                model: [1, 2]
                delegate: Rectangle {
                    id: slotChip
                    required property int modelData
                    readonly property int slotId: modelData === 1 ? Globals.mpv.primarySubId
                                                                  : Globals.mpv.secondarySubId
                    visible: slotId !== 0
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    radius: 8
                    color: Qt.alpha(Theme.accent, 0.14)

                    RowLayout {
                        anchors { fill: parent; leftMargin: 9; rightMargin: 6 }
                        spacing: 6

                        Text {
                            text: slotChip.modelData
                            color: Theme.accent
                            font.pixelSize: Globals.sp(13)
                            font.bold: true
                        }
                        Text {
                            Layout.fillWidth: true
                            text: Globals.mpv.subNameForId(slotChip.slotId)
                            color: Theme.textSecondary
                            font.pixelSize: Globals.sp(14)
                            elide: Text.ElideMiddle
                        }
                        AppIcon {
                            name: "x"
                            size: 13
                            color: clearOne.hovered ? Theme.danger : Theme.textMuted
                            HoverHandler { id: clearOne; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                onTapped: slotChip.modelData === 1 ? Globals.mpv.setPrimarySub(0)
                                                                   : Globals.mpv.setSecondarySub(0)
                            }
                        }
                    }
                }
            }

            Text {
                text: qsTr("Clear")
                color: clearBoth.hovered ? Theme.accent : Theme.textMuted
                font.pixelSize: Globals.sp(14)
                visible: Globals.mpv.primarySubId !== 0 && Globals.mpv.secondarySubId !== 0
                HoverHandler { id: clearBoth; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: Globals.mpv.clearSubs() }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 8
            currentIndex: {
                if (!panel.activeTab) return 0
                if (panel.activeTab.id === "servers") return 3
                if (panel.activeTab.id === "general") return 1
                if (panel.activeTab.id === "skip") return 2
                if (panel.activeTab.id === "subs" && panel.subsSubPage === 1) return 4
                return 0
            }

            Item {
                ListView {
                    id: listView
                    anchors {
                        fill: parent
                        margins: 8
                    }
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    spacing: 2

                    model: (panel.activeTab && panel.activeTab.type === "list")
                           ? panel.listModels[panel.activeTab.modelIndex] : null
                    currentIndex: model ? model.currentIndex : -1

                    header: Item {
                        width: listView.width
                        height: offRow.showing ? 40 : 0
                        visible: offRow.showing

                        Rectangle {
                            id: offRow
                            readonly property bool showing: panel.activeTab && panel.activeTab.modelIndex === 3
                            readonly property bool active: Globals.mpv.primarySubId === 0
                                                           && Globals.mpv.secondarySubId === 0
                            anchors { fill: parent; bottomMargin: 4 }
                            radius: 10
                            color: active            ? "#1A2550"
                                 : offHover.hovered  ? "#0Cffffff" : "transparent"

                            Text {
                                anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 14 }
                                text: qsTr("Off")
                                color: offRow.active ? Theme.accent : Theme.textSecondary
                                font.pixelSize: Globals.sp(19)
                                font.bold: offRow.active
                            }
                            AppIcon {
                                anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 12 }
                                visible: offRow.active
                                name: "check"
                                size: 16
                                color: Theme.accent
                            }
                            HoverHandler { id: offHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler { onTapped: Globals.mpv.clearSubs() }
                        }
                    }

                    onModelChanged: {
                        if (model) Qt.callLater(function() {
                            positionViewAtIndex(currentIndex, ListView.Center)
                        })
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                        width: 4
                        contentItem: Rectangle {
                            color: Theme.accent
                            radius: 2
                            opacity: 0.4
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: listView.count === 0
                        text: "No items"
                        color: "#9CA3AF"
                        font.pixelSize: Globals.sp(20)
                    }

                    delegate: AbstractButton {
                        id: serverBtn
                        required property string name
                        required property int index
                        readonly property bool isSubs: panel.activeTab && panel.activeTab.modelIndex === 3
                        readonly property int  secondary: isSubs && listView.model ? listView.model.secondaryIndex : -1
                        readonly property int  slotNumber: !isSubs ? 0
                                                         : index === listView.model.currentIndex ? 1
                                                         : index === secondary                   ? 2 : 0
                        property bool isCurrent: isSubs ? slotNumber > 0 : index === listView.currentIndex
                        readonly property bool isSecond: slotNumber === 2

                        width: listView.width
                        height: 44
                        focusPolicy: Qt.NoFocus

                        onClicked: {
                            if (!panel.activeTab || panel.activeTab.type !== "list") return
                            switch (panel.activeTab.modelIndex) {
                            case 0: App.playlist.loadServer(index); break
                            case 1: Globals.mpv.setVideoIndex(index); break
                            case 2: Globals.mpv.setAudioIndex(index); break
                            // Click sets primary; clicking the one already there turns it off.
                            case 3: serverBtn.slotNumber === 1 ? Globals.mpv.setPrimarySub(0)
                                                               : Globals.mpv.setSubIndex(index); break
                            }
                        }

                        background: Rectangle {
                            radius: 10
                            color: serverBtn.isCurrent ? "#1A2550"
                                 : serverBtn.hovered   ? "#0Cffffff"
                                 : "transparent"
                            border.color: serverBtn.isCurrent ? "#304E5BF2" : "transparent"
                            border.width: serverBtn.isCurrent ? 1 : 0
                            Behavior on color { ColorAnimation { duration: 100 } }

                            Rectangle {
                                visible: serverBtn.isCurrent
                                width: 3
                                radius: 1.5
                                color: Theme.accent
                                anchors {
                                    left: parent.left
                                    top: parent.top
                                    bottom: parent.bottom
                                    leftMargin: 2
                                    topMargin: 8
                                    bottomMargin: 8
                                }
                            }
                        }

                        contentItem: RowLayout {
                            spacing: 10

                            Chip {   // the only way to reach the second slot
                                visible: serverBtn.isSubs && serverBtn.hovered && serverBtn.slotNumber !== 2
                                Layout.leftMargin: 4
                                label: "2"
                                filled: secondHover.hovered
                                HoverHandler { id: secondHover; cursorShape: Qt.PointingHandCursor }
                                TapHandler { onTapped: Globals.mpv.setSubIndex(serverBtn.index, true) }
                                AppToolTip { text: qsTr("Use as second subtitle"); visible: secondHover.hovered }
                            }

                            Rectangle {
                                visible: serverBtn.isSubs && serverBtn.isCurrent
                                Layout.leftMargin: 4
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                radius: 5
                                color: Qt.alpha(Theme.accent, 0.22)
                                border.color: Theme.accent
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: serverBtn.isSecond ? "2" : "1"
                                    color: Theme.accent
                                    font.pixelSize: Globals.sp(13)
                                    font.bold: true
                                }
                            }

                            Item {
                                Layout.leftMargin: 10
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: serverBtn.isCurrent ? Theme.success : "#374151"
                                }

                                Rectangle {
                                    visible: serverBtn.isCurrent
                                    anchors.centerIn: parent
                                    width: 16
                                    height: 16
                                    radius: 8
                                    color: "#2010B981"
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: serverBtn.name
                                font.pixelSize: Globals.sp(20)
                                elide: Text.ElideRight
                                color: serverBtn.isCurrent ? "#E6FFE8"
                                     : serverBtn.hovered   ? "white"
                                     : "#9CA3AF"
                            }

                            AppIcon {
                                visible: serverBtn.isCurrent
                                name: "check"
                                size: 18
                                color: Theme.success
                                Layout.rightMargin: 10
                            }
                        }

                        scale: serverBtn.down ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 80 } }
                    }
                }
            }

            Item {
                Flickable {
                    anchors {
                        fill: parent
                        margins: 12
                    }
                    contentHeight: generalCol.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: generalCol
                        width: parent.width
                        spacing: 6

                        component SettingRow: Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            radius: 10
                            color: settingHover.hovered ? "#0Cffffff" : "transparent"
                            Behavior on color { ColorAnimation { duration: 100 } }

                            property alias label: settingLabel.text
                            default property alias content: settingSlot.children

                            HoverHandler { id: settingHover }

                            RowLayout {
                                anchors {
                                    fill: parent
                                    leftMargin: 14
                                    rightMargin: 14
                                }
                                spacing: 12

                                Text {
                                    id: settingLabel
                                    color: "#C7CEDB"
                                    font.pixelSize: Globals.sp(20)
                                    Layout.fillWidth: true
                                }

                                Item {
                                    id: settingSlot
                                    Layout.preferredWidth: childrenRect.width
                                    Layout.preferredHeight: parent.height
                                }
                            }
                        }

                        // The panel floats over video, so it keeps fixed greys and its own insets.
                        component PanelSlider: LabeledSlider {
                            stacked: true
                            labelColor: "#9AA3B5"
                            Layout.leftMargin: 14
                            Layout.rightMargin: 14
                            Layout.topMargin: 6
                        }

                        SettingRow {
                            label: qsTr("Subtitles")
                            AppSwitch {
                                anchors.verticalCenter: parent.verticalCenter
                                focusPolicy: Qt.NoFocus
                                checked: Globals.mpv.subVisible
                                onToggled: Globals.mpv.subVisible = checked
                            }
                        }

                        SettingRow {
                            label: qsTr("Mute")
                            AppSwitch {
                                anchors.verticalCenter: parent.verticalCenter
                                focusPolicy: Qt.NoFocus
                                checked: Globals.mpv.muted
                                onToggled: Globals.mpv.muted = checked
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8
                            Layout.preferredHeight: 1
                            color: "#0Cffffff"
                        }

                        PanelSlider {
                            label: qsTr("Volume")
                            from: 0; to: 200
                            unitSuffix: "%"
                            value: Globals.mpv.volume
                            onMoved: (v) => Globals.mpv.volume = v
                        }

                        PanelSlider {
                            label: qsTr("Speed")
                            from: 0.1; to: 4.0; stepSize: 0.05
                            unitSuffix: "x"; decimals: 2
                            value: Globals.mpv.speed
                            onMoved: (v) => Globals.mpv.speed = v
                        }

                        PanelSlider {
                            label: qsTr("Sub Size")
                            from: 20; to: 80; stepSize: 1
                            unitSuffix: "px"
                            value: App.settings.subFontSize
                            onMoved: (v) => {
                                // sub-scale resizes ASS + text subs; 40 = 1.0× so the value reads as px.
                                Globals.mpv.setProperty("sub-scale", v / 40.0)
                                App.settings.subFontSize = v
                            }
                        }

                        PanelSlider {
                            label: qsTr("Sub Position")
                            from: 0; to: 100; stepSize: 1
                            unitSuffix: "%"
                            value: App.settings.subPos
                            onMoved: (v) => {
                                Globals.mpv.setSubPos(v)
                                App.settings.subPos = v
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 56
                            radius: 10
                            color: "transparent"

                            ColumnLayout {
                                anchors { fill: parent; leftMargin: 14; rightMargin: 14 }
                                spacing: 2

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        text: qsTr("Sub Delay")
                                        color: "#9AA3B5"
                                        font.pixelSize: Globals.sp(20)
                                    }

                                    Item { Layout.fillWidth: true }

                                    Text {
                                        text: qsTr("Reset")
                                        color: resetDelayArea.containsMouse ? Theme.accent : "#7A8396"
                                        font.pixelSize: Globals.sp(16)
                                        visible: Globals.mpv.subDelay !== 0

                                        MouseArea {
                                            id: resetDelayArea
                                            anchors.fill: parent
                                            anchors.margins: -6
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: Globals.mpv.subDelay = 0
                                        }
                                    }
                                }

                                AppSlider {
                                    id: subDelaySlider
                                    Layout.fillWidth: true
                                    from: -60; to: 60; stepSize: 0.1   // matches the clamp in setSubDelay
                                    // Without this, stepSize only applies to keys and wheel; dragging stays continuous.
                                    snapMode: Slider.SnapAlways
                                    value: Globals.mpv.subDelay
                                    unitSuffix: "s"
                                    decimals: 1
                                    onMoved: Globals.mpv.subDelay = value

                                    // Dragging drops the binding above, and the handle then ignores Reset and mpv's z/Z.
                                    Connections {
                                        target: Globals.mpv
                                        function onSubDelayChanged() {
                                            if (!subDelaySlider.pressed)
                                                subDelaySlider.value = Globals.mpv.subDelay
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: 6
                            implicitHeight: 1
                            color: "#1Affffff"
                        }

                        Text {
                            Layout.leftMargin: 14
                            Layout.topMargin: 4
                            text: qsTr("Danmaku")
                            color: "#C7CEDB"
                            font.pixelSize: Globals.sp(20)
                            font.bold: true
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.leftMargin: 14
                            Layout.rightMargin: 14
                            text: qsTr("Independent of Sub Size and Sub Position.")
                            color: "#7A8396"
                            font.pixelSize: Globals.sp(15)
                            wrapMode: Text.WordWrap
                        }

                        PanelSlider {
                            label: qsTr("Danmaku Opacity")
                            from: 10; to: 100; stepSize: 5
                            unitSuffix: "%"
                            value: App.settings.danmakuOpacity
                            onMoved: (v) => App.settings.danmakuOpacity = v
                        }

                        PanelSlider {
                            label: qsTr("Danmaku Size")
                            from: 50; to: 200; stepSize: 10
                            unitSuffix: "%"
                            value: App.settings.danmakuFontScale
                            onMoved: (v) => App.settings.danmakuFontScale = v
                        }

                        PanelSlider {
                            label: qsTr("Danmaku Speed")
                            from: 25; to: 400; stepSize: 25
                            unitSuffix: "%"
                            value: App.settings.danmakuSpeed
                            onMoved: (v) => App.settings.danmakuSpeed = v
                        }

                        PanelSlider {
                            label: qsTr("Danmaku Area")
                            from: 10; to: 100; stepSize: 5
                            unitSuffix: "%"
                            value: App.settings.danmakuArea
                            onMoved: (v) => App.settings.danmakuArea = v
                        }

                        PanelSlider {
                            label: qsTr("Danmaku Density")
                            from: 0; to: 200; stepSize: 10
                            value: App.settings.danmakuMaxOnScreen
                            onMoved: (v) => App.settings.danmakuMaxOnScreen = v
                        }

                        SettingRow {
                            label: qsTr("Bold Danmaku")
                            AppSwitch {
                                anchors.verticalCenter: parent.verticalCenter
                                focusPolicy: Qt.NoFocus
                                checked: App.settings.danmakuBold
                                onToggled: App.settings.danmakuBold = checked
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                            implicitHeight: 40

                            AppButton {
                                anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                                text: qsTr("Reset Danmaku Appearance")
                                backgroundDefaultColor: Theme.surfaceAlt
                                contentItemTextColor: Theme.textPrimary
                                onClicked: App.settings.resetDanmakuAppearance()
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            Item {
                component SkipCard: Rectangle {
                    property string label
                    property bool   active
                    default property alias content: extraSlot.data

                    signal cardToggled()

                    Layout.fillWidth: true
                    implicitHeight: skipContent.implicitHeight + 20
                    radius: 12
                    color: "#0Affffff"
                    border.color: active ? Theme.accent : "#10ffffff"
                    border.width: 1
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    ColumnLayout {
                        id: skipContent
                        anchors {
                            fill: parent
                            margins: 10
                        }
                        spacing: 8

                        RowLayout {
                            spacing: 8
                            Text {
                                text: label
                                color: "#C7CEDB"
                                font.pixelSize: Globals.sp(20)
                                Layout.fillWidth: true
                            }
                            AppCheckBox {
                                focusPolicy: Qt.NoFocus
                                checked: active
                                onToggled: cardToggled()
                            }
                        }

                        Item {
                            id: extraSlot
                            Layout.fillWidth: true
                            implicitHeight: childrenRect.height
                        }
                    }
                }

                Flickable {
                    anchors {
                        fill: parent
                        margins: 12
                    }
                    contentHeight: skipCol.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: skipCol
                        width: parent.width
                        spacing: 8

                        Rectangle {
                            id: aniskipCard
                            Layout.fillWidth: true
                            implicitHeight: cardCol.implicitHeight + 20
                            radius: 10
                            readonly property bool aniskipOn: App.settings.aniskipEnabled
                            readonly property bool detected: aniskipOn && (Globals.mpv.hasOP || Globals.mpv.hasED)
                            color: detected ? "#1410B981" : "#0Affffff"
                            border.color: detected ? Theme.success : "#12ffffff"
                            border.width: 1
                            Behavior on border.color { ColorAnimation { duration: 150 } }

                            ColumnLayout {
                                id: cardCol
                                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
                                          leftMargin: 12; rightMargin: 10 }
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Rectangle {
                                        Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
                                        color: aniskipCard.detected ? Theme.success
                                             : aniskipCard.aniskipOn ? "#9AA3B5" : "#3D4658"
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: !aniskipCard.aniskipOn ? "AniSkip · off"
                                              : (App.skip.status.length > 0 ? App.skip.status : "AniSkip")
                                        color: "#C7CEDB"
                                        font.pixelSize: Globals.sp(17)
                                        elide: Text.ElideRight
                                    }
                                    AppSpinner {
                                        visible: App.skip.busy
                                        running: App.skip.busy
                                        radius: 7; dotSize: 4; dotCount: 8
                                        Layout.preferredWidth: 18; Layout.preferredHeight: 18
                                    }
                                    AppSwitch {
                                        focusPolicy: Qt.NoFocus
                                        checked: App.settings.aniskipEnabled
                                        onToggled: App.settings.aniskipEnabled = checked
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    visible: aniskipCard.aniskipOn
                                    spacing: 6

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 0
                                            Text { text: "Auto-skip"; color: "#C7CEDB"; font.pixelSize: Globals.sp(17) }
                                            Text { text: "Jump past intro & outro automatically"; color: "#9AA3B5"; font.pixelSize: Globals.sp(13) }
                                        }
                                        AppSwitch {
                                            focusPolicy: Qt.NoFocus
                                            checked: App.settings.aniskipAuto
                                            onToggled: App.settings.aniskipAuto = checked
                                        }
                                    }

                                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#0Cffffff" }

                                    Text {
                                        text: "Search query"
                                        color: "#9AA3B5"
                                        font.pixelSize: Globals.sp(14)
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6
                                        AppTextField {
                                            id: queryField
                                            Layout.fillWidth: true
                                            fontSize: 16
                                            placeholderText: "Search title..."
                                            Component.onCompleted: text = App.skip.searchQuery
                                            onAccepted: App.skip.searchQuery = text
                                            Connections {
                                                target: App.skip
                                                function onSearchQueryChanged() {
                                                    if (!queryField.activeFocus) queryField.text = App.skip.searchQuery
                                                }
                                            }
                                        }
                                        AppButton {
                                            id: researchBtn
                                            Layout.preferredWidth: 38
                                            Layout.preferredHeight: 36
                                            text: ""
                                            AppToolTip { text: qsTr("Search again"); visible: researchBtn.hovered }
                                            onClicked: { App.skip.searchQuery = queryField.text; App.skip.research() }
                                            AppIcon { anchors.centerIn: parent; name: "refresh-cw"; size: 17; color: "white" }
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2
                                            Text { text: "Matched show"; color: "#9AA3B5"; font.pixelSize: Globals.sp(14) }
                                            AppComboBox {
                                                Layout.fillWidth: true
                                                text: ""
                                                fontSize: 15
                                                placeholderText: "No match"
                                                model: App.skip.showTitles
                                                currentIndex: App.skip.selectedShow
                                                onActivated: App.skip.selectedShow = currentIndex
                                            }
                                        }
                                        ColumnLayout {
                                            Layout.preferredWidth: 96
                                            spacing: 2
                                            Text { text: "Episode"; color: "#9AA3B5"; font.pixelSize: Globals.sp(14) }
                                            AppSpinBox {
                                                Layout.fillWidth: true
                                                from: 1
                                                to: Math.max(1, App.skip.episodeCount, App.skip.selectedEpisode)
                                                value: App.skip.selectedEpisode
                                                stepSize: 1
                                                focusPolicy: Qt.NoFocus
                                                onValueModified: App.skip.selectedEpisode = value
                                            }
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.topMargin: 2
                                        spacing: 8
                                        visible: App.skip.introRange.length > 0 || App.skip.outroRange.length > 0

                                        Repeater {
                                            model: [
                                                { tag: "Intro", range: App.skip.introRange },
                                                { tag: "Outro", range: App.skip.outroRange }
                                            ]
                                            delegate: Rectangle {
                                                required property var modelData
                                                visible: modelData.range.length > 0
                                                Layout.fillWidth: true
                                                implicitHeight: 30
                                                radius: 8
                                                color: "#1410B981"
                                                border.color: "#3010B981"
                                                border.width: 1
                                                Row {
                                                    anchors.centerIn: parent
                                                    spacing: 6
                                                    Text {
                                                        text: modelData.tag
                                                        color: Theme.success
                                                        font { pixelSize: Globals.sp(13); weight: Font.DemiBold; letterSpacing: 0.5 }
                                                        anchors.verticalCenter: parent.verticalCenter
                                                    }
                                                    Text {
                                                        text: modelData.range
                                                        color: "white"
                                                        font.pixelSize: Globals.sp(15)
                                                        anchors.verticalCenter: parent.verticalCenter
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        SkipCard {
                            label: "Skip Opening"
                            active: Globals.mpv.skipOP
                            onCardToggled: Globals.mpv.skipOP = !Globals.mpv.skipOP

                            RowLayout {
                                width: parent.width
                                spacing: 8
                                Text { text: "Start";  color: "#9AA3B5"; font.pixelSize: Globals.sp(20) }
                                AppSpinBox {
                                    Layout.fillWidth: true
                                    value: Globals.mpv.skipOPStart
                                    from: 0
                                    to: Globals.mpv.duration
                                    focusPolicy: Qt.NoFocus
                                    stepSize: 10
                                    onValueModified: Globals.mpv.skipOPStart = value
                                }
                                Text { text: "Length"; color: "#9AA3B5"; font.pixelSize: Globals.sp(20) }
                                AppSpinBox {
                                    Layout.fillWidth: true
                                    value: Globals.mpv.skipOPLength
                                    from: 0
                                    to: Globals.mpv.duration
                                    focusPolicy: Qt.NoFocus
                                    stepSize: 10
                                    onValueModified: Globals.mpv.skipOPLength = value
                                }
                            }
                        }

                        SkipCard {
                            label: "Skip Ending"
                            active: Globals.mpv.skipED
                            onCardToggled: Globals.mpv.skipED = !Globals.mpv.skipED

                            RowLayout {
                                width: parent.width
                                spacing: 8
                                Text { text: "Length"; color: "#9AA3B5"; font.pixelSize: Globals.sp(20) }
                                AppSpinBox {
                                    Layout.fillWidth: true
                                    value: Globals.mpv.skipEDLength
                                    from: 0
                                    to: Globals.mpv.duration
                                    focusPolicy: Qt.NoFocus
                                    stepSize: 10
                                    onValueModified: Globals.mpv.skipEDLength = value
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            radius: 12
                            color: (Globals.mpv.skipOP && Globals.mpv.skipED) ? "#0Fffffff" : "#0Affffff"
                            border.color: (Globals.mpv.skipOP && Globals.mpv.skipED) ? Theme.accent : "#10ffffff"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on border.color { ColorAnimation { duration: 150 } }

                            RowLayout {
                                anchors {
                                    fill: parent
                                    leftMargin: 14
                                    rightMargin: 14
                                }
                                spacing: 8

                                Text {
                                    text: "Skip Both"
                                    color: "#C7CEDB"
                                    font.pixelSize: Globals.sp(20)
                                    Layout.fillWidth: true
                                }

                                AppCheckBox {
                                    id: skipBothCb
                                    focusPolicy: Qt.NoFocus
                                    checked: Globals.mpv.skipED && Globals.mpv.skipOP
                                    onToggled: {
                                        let v = !(Globals.mpv.skipED && Globals.mpv.skipOP)
                                        Globals.mpv.skipED = v
                                        Globals.mpv.skipOP = v
                                    }
                                    AppToolTip { text: qsTr("Toggle both OP and ED skip"); visible: skipBothCb.hovered }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            Item {
                ListView {
                    id: serverListView
                    anchors { fill: parent; margins: 8 }
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    spacing: 2
                    model: App.playlist.serverList
                    currentIndex: model ? model.currentIndex : -1

                    onCountChanged: if (model) Qt.callLater(() => positionViewAtIndex(currentIndex, ListView.Center))

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                        width: 4
                        contentItem: Rectangle { color: Theme.accent; radius: 2; opacity: 0.4 }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: serverListView.count === 0
                        text: "No servers"
                        color: "#9CA3AF"
                        font.pixelSize: Globals.sp(20)
                    }

                    // Model supplies the section key (Subbed/Dubbed/Broken, or "" -> no header).
                    section.property: "section"
                    section.criteria: ViewSection.FullString
                    section.delegate: Item {
                        required property string section
                        width: serverListView.width
                        height: section === "" ? 0 : 26
                        visible: section !== ""
                        Text {
                            anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                            text: parent.section
                            color: parent.section === "Broken" ? Theme.danger : Theme.textAccent
                            font { pixelSize: Globals.sp(15); weight: Font.DemiBold; letterSpacing: 1 }
                        }
                    }

                    delegate: AbstractButton {
                        id: srvBtn
                        required property string name
                        required property int index
                        required property int status        // 0 unchecked, 1 working, 2 broken
                        property bool isCurrent: index === serverListView.currentIndex

                        width: serverListView.width
                        height: 44
                        focusPolicy: Qt.NoFocus
                        opacity: status === 2 ? 0.5 : 1.0
                        enabled: status !== 2          // broken servers are non-selectable
                        onClicked: App.playlist.loadServer(index)

                        background: Rectangle {
                            radius: 10
                            color: srvBtn.isCurrent ? "#1A2550"
                                 : srvBtn.hovered   ? "#0Cffffff"
                                 : "transparent"
                            border.color: srvBtn.isCurrent ? "#304E5BF2" : "transparent"
                            border.width: srvBtn.isCurrent ? 1 : 0
                            Behavior on color { ColorAnimation { duration: 100 } }

                            Rectangle {
                                visible: srvBtn.isCurrent
                                width: 3; radius: 1.5
                                color: Theme.accent
                                anchors { left: parent.left; top: parent.top; bottom: parent.bottom
                                          leftMargin: 2; topMargin: 8; bottomMargin: 8 }
                            }
                        }

                        contentItem: RowLayout {
                            spacing: 10
                            Item {
                                Layout.leftMargin: 10
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                Rectangle {
                                    anchors.centerIn: parent
                                    Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
                                    color: (srvBtn.isCurrent || srvBtn.status === 1) ? Theme.success
                                         : srvBtn.status === 2                        ? Theme.danger
                                         : "#4B5563"   // unchecked / still checking
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: srvBtn.name
                                font.pixelSize: Globals.sp(20)
                                elide: Text.ElideRight
                                color: srvBtn.isCurrent  ? "#E6FFE8"
                                     : srvBtn.status === 2 ? "#6B7280"
                                     : srvBtn.hovered     ? "white"
                                     : "#9CA3AF"
                            }
                            AppIcon {
                                visible: srvBtn.isCurrent || srvBtn.status === 2
                                name: srvBtn.isCurrent ? "check" : "x"
                                size: 16
                                color: srvBtn.isCurrent ? Theme.success : Theme.danger
                                Layout.rightMargin: 10
                            }
                        }

                        scale: srvBtn.down ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 80 } }
                    }
                }
            }

            // Its own list on purpose: a result joins the track list only once picked.
            Item {
                ColumnLayout {
                    anchors { fill: parent; margins: 10 }
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        AppTextField {
                            id: subQueryField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Search subtitles by title")
                            showClearButton: true
                            fontSize: 18
                            onAccepted: App.subtitleSearch.search(text)

                            // currentShowName() has no notifier, so refill on entry rather than bind.
                            property string prefilled: ""
                            function offerShowName() {
                                if (text !== "" && text !== prefilled) return
                                prefilled = App.playlist.currentShowName()
                                text = prefilled
                            }
                            Component.onCompleted: offerShowName()
                        }

                        AppButton {
                            text: qsTr("Search")
                            cornerRadius: 8
                            fontSize: 18
                            enabled: !App.subtitleSearch.isLoading
                            onClicked: App.subtitleSearch.search(subQueryField.text)
                        }
                    }

                    // SubDL takes a comma list, so one search returns every chosen language.
                    Flow {
                        id: subLangs
                        Layout.fillWidth: true
                        spacing: 6

                        readonly property var codes: ["EN", "ES", "FR", "DE", "PT", "AR", "ZH", "JA", "KO"]
                        property var chosen: []

                        function load() {
                            const saved = App.settings.getString("subtitles/subdlLanguages", "EN")
                            chosen = saved.split(",").filter(c => c !== "")
                        }
                        function toggle(code) {
                            let next = chosen.slice()
                            const at = next.indexOf(code)
                            if (at >= 0) next.splice(at, 1)
                            else next.push(code)
                            if (next.length === 0) return   // never leave every language off
                            chosen = next
                            App.settings.setString("subtitles/subdlLanguages", next.join(","))
                            if (!App.subtitleSearch.isLoading)
                                App.subtitleSearch.search(subQueryField.text)
                        }
                        Component.onCompleted: load()

                        Repeater {
                            model: subLangs.codes
                            delegate: Chip {
                                id: langChip
                                required property string modelData
                                label: modelData
                                filled: subLangs.chosen.indexOf(modelData) >= 0
                                enabled: !App.subtitleSearch.isLoading
                                opacity: enabled ? 1.0 : 0.5
                                HoverHandler { cursorShape: Qt.PointingHandCursor }
                                TapHandler { onTapped: subLangs.toggle(langChip.modelData) }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: App.subtitleSearch.count === 0
                        wrapMode: Text.Wrap
                        color: "#7A8396"
                        font.pixelSize: Globals.sp(18)
                        text: App.subtitleSearch.isLoading  ? qsTr("Searching...")
                            : App.subtitleSearch.query === "" ? qsTr("Search SubDL for a subtitle to use.")
                            : qsTr("Nothing found.")
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 4
                        model: App.subtitleSearch
                        ScrollBar.vertical: AppScrollBar {}

                        delegate: Rectangle {
                            id: subResult
                            required property string displayName
                            required property string release
                            required property string language
                            required property string author
                            required property string episodeLabel
                            required property var    tags
                            required property bool   hearingImpaired
                            required property int    slot
                            required property bool   fetching
                            required property int    index

                            readonly property var chips: {
                                let c = []
                                if (episodeLabel !== "") c.push(episodeLabel)
                                if (language !== "") c.push(language)
                                for (const t of tags) c.push(t)
                                if (hearingImpaired) c.push("SDH")
                                return c
                            }

                            width: ListView.view.width
                            implicitHeight: subCol.implicitHeight + 20
                            radius: 10
                            color: subResult.slot > 0 ? Qt.alpha(Theme.accent, 0.16)
                                 : subHover.hovered   ? Qt.alpha(Theme.textPrimary, 0.07)
                                                      : "transparent"
                            Behavior on color { ColorAnimation { duration: 120 } }

                            Rectangle {
                                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                                width: 3
                                radius: 2
                                color: Theme.accent
                                visible: subResult.slot > 0
                            }

                            ColumnLayout {
                                id: subCol
                                anchors {
                                    left: parent.left; right: parent.right; top: parent.top
                                    leftMargin: 12; rightMargin: 12; topMargin: 10
                                }
                                spacing: 7

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        Layout.fillWidth: true
                                        text: subResult.displayName
                                        color: "#FFFFFF"
                                        font.pixelSize: Globals.sp(17)
                                        font.bold: subResult.slot > 0
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        visible: subResult.slot > 0
                                        text: subResult.slot === 1 ? qsTr("PRIMARY") : qsTr("SECONDARY")
                                        color: Theme.accent
                                        font.pixelSize: Globals.sp(12)
                                        font.bold: true
                                    }

                                    // Tapping the row takes slot 1; this is the only way to slot 2.
                                    Chip {
                                        visible: subHover.hovered && subResult.slot !== 2
                                        label: "2"
                                        filled: subSecondHover.hovered
                                        HoverHandler { id: subSecondHover; cursorShape: Qt.PointingHandCursor }
                                        TapHandler { onTapped: App.subtitleSearch.use(subResult.index, true) }
                                    }

                                    AppSpinner {
                                        visible: subResult.fetching
                                        running: subResult.fetching
                                        radius: 6
                                        dotSize: 3
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Repeater {
                                        model: subResult.chips
                                        delegate: Chip {
                                            required property string modelData
                                            label: modelData
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                        visible: subResult.author !== ""
                                        text: subResult.author
                                        color: Theme.textMuted
                                        font.pixelSize: Globals.sp(13)
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            HoverHandler { id: subHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                enabled: !subSecondHover.hovered   // the "2" chip wins its own taps
                                onTapped: App.subtitleSearch.use(subResult.index)
                            }
                        }
                    }
                }
            }
        }
    }
}