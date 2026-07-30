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
                if (c === 0) continue
                if (t.id === "servers" && c <= 1) continue   // one server -> nothing to pick
            }
            tabs.push(t)
        }
        return tabs
    }

    property int activeTabIndex: 0
    property string activeTabId: "servers"
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

    onOpened: syncActiveTab()

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
                            readonly property var tabData: panel.visibleTabs[index]
                            readonly property bool isActive: panel.activeTabIndex === index
                            readonly property bool isList: tabData.type === "list"
                            readonly property int itemCount: isList ? panel.listModels[tabData.modelIndex].count : -1

                            width: (tabRow.width - (panel.visibleTabs.length - 1) * 2) / panel.visibleTabs.length
                            height: tabRow.height
                            focusPolicy: Qt.NoFocus
                            onClicked: { panel.activeTabIndex = index; panel.activeTabId = panel.visibleTabs[index].id }

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
                                        id: tabMain
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
                id: tabIndicator
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

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 8
            currentIndex: {
                if (!panel.activeTab) return 0
                if (panel.activeTab.id === "servers") return 3
                if (panel.activeTab.id === "general") return 1
                if (panel.activeTab.id === "skip") return 2
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
                        property bool isCurrent: index === listView.currentIndex

                        width: listView.width
                        height: 44
                        focusPolicy: Qt.NoFocus

                        onClicked: {
                            if (!panel.activeTab || panel.activeTab.type !== "list") return
                            switch (panel.activeTab.modelIndex) {
                            case 0: App.playlist.loadServer(index); break
                            case 1: Globals.mpv.setVideoIndex(index); break
                            case 2: Globals.mpv.setAudioIndex(index); break
                            case 3: Globals.mpv.setSubIndex(index); break
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

                        SettingRow {
                            label: qsTr("Subtitles")
                            AppSwitch {
                                anchors.verticalCenter: parent.verticalCenter
                                focusPolicy: Qt.NoFocus
                                checked: mpv.subVisible
                                onToggled: mpv.subVisible = checked
                            }
                        }

                        SettingRow {
                            label: qsTr("Mute")
                            AppSwitch {
                                anchors.verticalCenter: parent.verticalCenter
                                focusPolicy: Qt.NoFocus
                                checked: mpv.muted
                                onToggled: mpv.muted = checked
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8
                            Layout.preferredHeight: 1
                            color: "#0Cffffff"
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 56
                            radius: 10
                            color: "transparent"

                            ColumnLayout {
                                anchors {
                                    fill: parent
                                    leftMargin: 14
                                    rightMargin: 14
                                }
                                spacing: 2

                                Text {
                                    text: qsTr("Volume")
                                    color: "#9AA3B5"
                                    font.pixelSize: Globals.sp(20)
                                }

                                AppSlider {
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 200
                                    value: mpv.volume
                                    unitSuffix: "%"
                                    decimals: 0
                                    onMoved: mpv.volume = value
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 56
                            radius: 10
                            color: "transparent"

                            ColumnLayout {
                                anchors {
                                    fill: parent
                                    leftMargin: 14
                                    rightMargin: 14
                                }
                                spacing: 2

                                Text {
                                    text: qsTr("Speed")
                                    color: "#9AA3B5"
                                    font.pixelSize: Globals.sp(20)
                                }

                                AppSlider {
                                    Layout.fillWidth: true
                                    from: 0.1
                                    to: 4.0
                                    stepSize: 0.05
                                    value: mpv.speed
                                    unitSuffix: "x"
                                    decimals: 2
                                    onMoved: mpv.speed = value
                                }
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

                                Text {
                                    text: qsTr("Sub Size")
                                    color: "#9AA3B5"
                                    font.pixelSize: Globals.sp(20)
                                }

                                AppSlider {
                                    id: subSizeSlider
                                    Layout.fillWidth: true
                                    from: 20; to: 80; stepSize: 1
                                    value: App.settings.subFontSize
                                    unitSuffix: "px"
                                    decimals: 0
                                    onMoved: {
                                        // sub-scale resizes ASS + text subs; 40 = 1.0× so the value reads as px.
                                        mpv.setProperty("sub-scale", value / 40.0)
                                        App.settings.subFontSize = value
                                    }
                                }
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

                                Text {
                                    text: qsTr("Sub Position")
                                    color: "#9AA3B5"
                                    font.pixelSize: Globals.sp(20)
                                }

                                AppSlider {
                                    id: subPosSlider
                                    Layout.fillWidth: true
                                    from: 0; to: 100; stepSize: 1
                                    value: App.settings.subPos
                                    unitSuffix: "%"
                                    decimals: 0
                                    onMoved: {
                                        mpv.setProperty("sub-pos", value)
                                        App.settings.subPos = value
                                    }
                                }
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
                            readonly property bool detected: aniskipOn && (mpv.hasOP || mpv.hasED)
                            color: detected ? "#1410B981" : "#0Affffff"
                            border.color: detected ? Theme.success : "#12ffffff"
                            border.width: 1
                            Behavior on border.color { ColorAnimation { duration: 150 } }

                            ColumnLayout {
                                id: cardCol
                                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
                                          leftMargin: 12; rightMargin: 10 }
                                spacing: 8

                                // Status row: dot · status text · spinner · master switch
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

                                // Match controls - editable query/show/episode for correcting a wrong match.
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
                            active: mpv.skipOP
                            onCardToggled: mpv.skipOP = !mpv.skipOP

                            RowLayout {
                                width: parent.width
                                spacing: 8
                                Text { text: "Start";  color: "#9AA3B5"; font.pixelSize: Globals.sp(20) }
                                AppSpinBox {
                                    Layout.fillWidth: true
                                    value: mpv.skipOPStart
                                    from: 0
                                    to: mpv.duration
                                    focusPolicy: Qt.NoFocus
                                    stepSize: 10
                                    onValueModified: mpv.skipOPStart = value
                                }
                                Text { text: "Length"; color: "#9AA3B5"; font.pixelSize: Globals.sp(20) }
                                AppSpinBox {
                                    Layout.fillWidth: true
                                    value: mpv.skipOPLength
                                    from: 0
                                    to: mpv.duration
                                    focusPolicy: Qt.NoFocus
                                    stepSize: 10
                                    onValueModified: mpv.skipOPLength = value
                                }
                            }
                        }

                        SkipCard {
                            label: "Skip Ending"
                            active: mpv.skipED
                            onCardToggled: mpv.skipED = !mpv.skipED

                            RowLayout {
                                width: parent.width
                                spacing: 8
                                Text { text: "Length"; color: "#9AA3B5"; font.pixelSize: Globals.sp(20) }
                                AppSpinBox {
                                    Layout.fillWidth: true
                                    value: mpv.skipEDLength
                                    from: 0
                                    to: mpv.duration
                                    focusPolicy: Qt.NoFocus
                                    stepSize: 10
                                    onValueModified: mpv.skipEDLength = value
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            radius: 12
                            color: (mpv.skipOP && mpv.skipED) ? "#0Fffffff" : "#0Affffff"
                            border.color: (mpv.skipOP && mpv.skipED) ? Theme.accent : "#10ffffff"
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
                                    checked: mpv.skipED && mpv.skipOP
                                    onToggled: {
                                        let v = !(mpv.skipED && mpv.skipOP)
                                        mpv.skipED = v
                                        mpv.skipOP = v
                                    }
                                    AppToolTip { text: qsTr("Toggle both OP and ED skip"); visible: skipBothCb.hovered }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // Servers - status-aware (working / broken / checking) with optional
            // Subbed / Dubbed section headers.
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
        }
    }
}