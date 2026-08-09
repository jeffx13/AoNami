import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AoNami
import QtQuick.Dialogs
import "../Components"
import ".."

Page {
    background: Rectangle { color: "transparent" }

    header: Item {
        height: 48
        Text {
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 16 }
            text: qsTr("Settings"); color: Theme.textPrimary; font { pixelSize: Globals.sp(24); bold: true }
        }
    }

    ColorDialog {
        id: accentDialog
        // selectedColor is a color; the setting is a string. QVariant won't convert color->string,
        // so stringify explicitly or the accent silently stays the theme default.
        onAccepted: App.settings.accentColor = selectedColor.toString()
    }

    Component {
        id: themeSwatch
        Rectangle {
            required property var modelData
            readonly property var p: Theme.palettes[modelData.name]
            readonly property bool active: App.settings.themeName === modelData.name
            width: 132; height: 78; radius: 10
            gradient: Gradient {
                GradientStop { position: 0.0; color: p.background }
                GradientStop { position: 1.0; color: p.bgBottom }
            }
            border.color: active ? Theme.accent : p.border
            border.width: active ? 2 : 1

            Column {
                anchors { fill: parent; margins: 10 }
                spacing: 8
                Row {
                    spacing: 6
                    Rectangle { width: 26; height: 14; radius: 4; color: p.surface; border.color: p.border; border.width: 1 }
                    Rectangle { width: 42; height: 14; radius: 4; color: p.accent }
                }
                Text { text: modelData.label; color: p.textPrimary; font.pixelSize: Globals.sp(15) }
            }
            AppIcon {
                visible: active
                anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 8 }
                name: "check"; size: 16; color: Theme.accent
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: App.settings.themeName = modelData.name
            }
        }
    }


    component SettingsCard: Rectangle {
        property alias title: titleText.text
        default property alias content: cardCol.children
        Layout.fillWidth: true
        implicitHeight: cardCol.implicitHeight + 24
        radius: 10; color: Theme.surface; border.color: Theme.border; border.width: 1

        ColumnLayout {
            id: cardCol
            anchors { fill: parent; margins: 12 }
            spacing: 10
            Text { id: titleText; color: Theme.accent; font { pixelSize: Globals.sp(20); bold: true } }
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: rootCol.implicitHeight + 32
        clip: true; boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: rootCol
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
            spacing: 12

            SettingsCard {
                title: qsTr("Appearance")

                Text { text: qsTr("Theme"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }

                Text { text: qsTr("Dark"); color: Theme.textMuted; font.pixelSize: Globals.sp(17) }
                Flow {
                    Layout.fillWidth: true
                    spacing: 10
                    Repeater {
                        model: [
                            { name: "obsidian", label: "Obsidian" },
                            { name: "amethyst", label: "Amethyst" },
                            { name: "slate",    label: "Slate" },
                            { name: "glacier",  label: "Glacier" },
                            { name: "onyx",     label: "Onyx" }
                        ]
                        delegate: themeSwatch
                    }
                }

                Text { text: qsTr("Light"); color: Theme.textMuted; font.pixelSize: Globals.sp(17) }
                Flow {
                    Layout.fillWidth: true
                    spacing: 10
                    Repeater {
                        model: [
                            { name: "grass",    label: "Grass" },
                            { name: "seafoam",  label: "Seafoam" },
                            { name: "frost",    label: "Frost" },
                            { name: "lavender", label: "Lavender" },
                            { name: "mist",     label: "Mist" }
                        ]
                        delegate: themeSwatch
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 10
                    Text { text: qsTr("Accent"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                    Rectangle { Layout.preferredWidth: 28; Layout.preferredHeight: 28; radius: 6; color: Theme.accent; border.color: Theme.border; border.width: 1 }
                    Item { Layout.fillWidth: true }
                    AppButton { text: qsTr("Change"); onClicked: { accentDialog.selectedColor = Theme.accent; accentDialog.open() } }
                    AppButton { text: qsTr("Reset"); backgroundDefaultColor: Theme.surfaceAlt; contentItemTextColor: Theme.textPrimary; onClicked: App.settings.accentColor = "" }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    ColumnLayout {
                        spacing: 0
                        Text { text: qsTr("UI Scale"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                        Text { text: qsTr("Size of text and controls across the app"); color: Theme.textMuted; font.pixelSize: Globals.sp(15) }
                    }
                    AppSlider {
                        Layout.fillWidth: true
                        from: 0.8; to: 1.4; stepSize: 0.05
                        value: App.settings.uiScale
                        unitSuffix: "x"
                        decimals: 2
                        onMoved: App.settings.uiScale = value
                    }
                }
            }

            SettingsCard {
                title: qsTr("General")

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: qsTr("MPV Logs"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20); Layout.fillWidth: true }
                    AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.mpvLogEnabled; onToggled: App.settings.mpvLogEnabled = checked }
                }
                RowLayout {
                    Layout.fillWidth: true
                    AppButton { text: qsTr("Clear Logs"); onClicked: App.logList.clear() }
                    Item { Layout.fillWidth: true }
                }
            }

            SettingsCard {
                title: qsTr("Network")

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: qsTr("Proxy"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                    AppTextField {
                        id: proxyField; text: App.settings.proxy
                        placeholderText: qsTr("http://127.0.0.1:7890")
                        Layout.fillWidth: true
                        onEditingFinished: App.settings.proxy = text
                    }
                    AppButton { text: qsTr("Apply"); onClicked: App.settings.proxy = proxyField.text }
                    AppButton { text: qsTr("Clear"); backgroundDefaultColor: Theme.surfaceAlt; contentItemTextColor: Theme.textPrimary; onClicked: { proxyField.text = ""; App.settings.proxy = "" } }
                }
            }

            SettingsCard {
                title: qsTr("Player")

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: qsTr("Use yt-dlp"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20); Layout.fillWidth: true }
                    AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.mpvYtdlEnabled; onToggled: App.settings.mpvYtdlEnabled = checked }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    ColumnLayout {
                        spacing: 0
                        Text { text: qsTr("Prefer Dubbed Audio"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                        Text { text: qsTr("Try dub servers first (off = subbed)"); color: Theme.textMuted; font.pixelSize: Globals.sp(15) }
                    }
                    Item { Layout.fillWidth: true }
                    AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.preferDub; onToggled: App.settings.preferDub = checked }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    ColumnLayout {
                        spacing: 0
                        Text { text: qsTr("Mark Watched At"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                        Text { text: qsTr("Episode counts as watched past this much"); color: Theme.textMuted; font.pixelSize: Globals.sp(15) }
                    }
                    AppSlider {
                        Layout.fillWidth: true
                        from: 0; to: 100; stepSize: 5
                        unitSuffix: "%"
                        value: App.settings.watchedPercent
                        onMoved: App.settings.watchedPercent = value
                    }
                }

                Flow {
                    Layout.fillWidth: true; spacing: 6
                    AppButton { text: qsTr("mpv folder"); backgroundDefaultColor: Theme.surfaceAlt; contentItemTextColor: Theme.textPrimary; onClicked: Qt.openUrlExternally("file:///" + App.settings.appDir + "/mpv") }
                    AppButton { text: qsTr("mpv.conf"); backgroundDefaultColor: Theme.surfaceAlt; contentItemTextColor: Theme.textPrimary; onClicked: Qt.openUrlExternally("file:///" + App.settings.appDir + "/mpv/mpv.conf") }
                    AppButton { text: qsTr("settings.ini"); backgroundDefaultColor: Theme.surfaceAlt; contentItemTextColor: Theme.textPrimary; onClicked: Qt.openUrlExternally(App.settings.path) }
                }
            }

            SettingsCard {
                title: qsTr("Danmaku")

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    ColumnLayout {
                        spacing: 0
                        Text { text: qsTr("Show Danmaku"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                        Text { text: qsTr("Bullet comments, where the provider has them"); color: Theme.textMuted; font.pixelSize: Globals.sp(15) }
                    }
                    Item { Layout.fillWidth: true }
                    AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.danmakuEnabled; onToggled: App.settings.danmakuEnabled = checked }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    Text { text: qsTr("Opacity"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                    AppSlider {
                        Layout.fillWidth: true
                        from: 10; to: 100; stepSize: 5
                        unitSuffix: "%"
                        value: App.settings.danmakuOpacity
                        onMoved: App.settings.danmakuOpacity = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    Text { text: qsTr("Font Size"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                    AppSlider {
                        Layout.fillWidth: true
                        from: 50; to: 200; stepSize: 10
                        unitSuffix: "%"
                        value: App.settings.danmakuFontScale
                        onMoved: App.settings.danmakuFontScale = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    Text { text: qsTr("Speed"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                    AppSlider {
                        Layout.fillWidth: true
                        from: 25; to: 400; stepSize: 25
                        unitSuffix: "%"
                        value: App.settings.danmakuSpeed
                        onMoved: App.settings.danmakuSpeed = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    ColumnLayout {
                        spacing: 0
                        Text { text: qsTr("Screen Area"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                        Text { text: qsTr("How far down the frame comments may go"); color: Theme.textMuted; font.pixelSize: Globals.sp(15) }
                    }
                    AppSlider {
                        Layout.fillWidth: true
                        from: 10; to: 100; stepSize: 5
                        unitSuffix: "%"
                        value: App.settings.danmakuArea
                        onMoved: App.settings.danmakuArea = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    ColumnLayout {
                        spacing: 0
                        Text { text: qsTr("Max On Screen"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                        Text { text: qsTr("0 for no limit"); color: Theme.textMuted; font.pixelSize: Globals.sp(15) }
                    }
                    AppSlider {
                        Layout.fillWidth: true
                        from: 0; to: 200; stepSize: 10
                        value: App.settings.danmakuMaxOnScreen
                        onMoved: App.settings.danmakuMaxOnScreen = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    ColumnLayout {
                        spacing: 0
                        Text { text: qsTr("Hide Spam"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                        Text { text: qsTr("Drop comments rated below this (0 keeps all)"); color: Theme.textMuted; font.pixelSize: Globals.sp(15) }
                    }
                    AppSlider {
                        Layout.fillWidth: true
                        from: 0; to: 11; stepSize: 1
                        value: App.settings.danmakuMinWeight
                        onMoved: App.settings.danmakuMinWeight = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: qsTr("Outline"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20); Layout.fillWidth: true }
                    AppComboBox {
                        Layout.preferredWidth: 160
                        model: [qsTr("None"), qsTr("Outline"), qsTr("Outline + Shadow")]
                        currentIndex: App.settings.danmakuOutline
                        onActivated: App.settings.danmakuOutline = currentIndex
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    text: qsTr("Hide comment types")
                    color: Theme.textSecondary
                    font.pixelSize: Globals.sp(20)
                }

                Flow {
                    Layout.fillWidth: true; spacing: 14

                    Row {
                        spacing: 6
                        Text { text: qsTr("Scrolling"); color: Theme.textMuted; font.pixelSize: Globals.sp(17); anchors.verticalCenter: parent.verticalCenter }
                        AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.danmakuBlockScroll; onToggled: App.settings.danmakuBlockScroll = checked }
                    }
                    Row {
                        spacing: 6
                        Text { text: qsTr("Top"); color: Theme.textMuted; font.pixelSize: Globals.sp(17); anchors.verticalCenter: parent.verticalCenter }
                        AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.danmakuBlockTop; onToggled: App.settings.danmakuBlockTop = checked }
                    }
                    Row {
                        spacing: 6
                        Text { text: qsTr("Bottom"); color: Theme.textMuted; font.pixelSize: Globals.sp(17); anchors.verticalCenter: parent.verticalCenter }
                        AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.danmakuBlockBottom; onToggled: App.settings.danmakuBlockBottom = checked }
                    }
                    Row {
                        spacing: 6
                        Text { text: qsTr("Colour"); color: Theme.textMuted; font.pixelSize: Globals.sp(17); anchors.verticalCenter: parent.verticalCenter }
                        AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.danmakuBlockColour; onToggled: App.settings.danmakuBlockColour = checked }
                    }
                    Row {
                        spacing: 6
                        Text { text: qsTr("Repeats"); color: Theme.textMuted; font.pixelSize: Globals.sp(17); anchors.verticalCenter: parent.verticalCenter }
                        AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.danmakuBlockRepeat; onToggled: App.settings.danmakuBlockRepeat = checked }
                    }
                }

                Flow {
                    Layout.fillWidth: true; spacing: 6
                    AppButton {
                        text: qsTr("Reset Appearance")
                        backgroundDefaultColor: Theme.surfaceAlt
                        contentItemTextColor: Theme.textPrimary
                        onClicked: App.settings.resetDanmakuAppearance()
                    }
                }
            }

            SettingsCard {
                title: qsTr("Downloads")

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: qsTr("Concurrent Downloads"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20); Layout.fillWidth: true }
                    AppSpinBox {
                        from: 1; to: 8
                        value: App.downloader.maxDownloads
                        onValueModified: App.downloader.maxDownloads = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: qsTr("Speed Limit"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20); Layout.fillWidth: true }
                    AppTextField {
                        placeholderText: qsTr("e.g. 5M  (blank = unlimited)")
                        text: App.settings.maxSpeed
                        implicitWidth: 200
                        onEditingFinished: App.settings.maxSpeed = text
                    }
                }
            }

            SettingsCard {
                title: qsTr("Integrations")

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    ColumnLayout {
                        spacing: 0
                        Text { text: qsTr("Discord Rich Presence"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                        Text { text: qsTr("Show what you're watching on Discord"); color: Theme.textMuted; font.pixelSize: Globals.sp(15) }
                    }
                    Item { Layout.fillWidth: true }
                    AppSwitch { focusPolicy: Qt.NoFocus; checked: App.settings.discordEnabled; onToggled: App.settings.discordEnabled = checked }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}