import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AoNami
import QtCore
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
        onAccepted: App.settings.accentColor = selectedColor
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

                Flow {
                    Layout.fillWidth: true
                    spacing: 10
                    Repeater {
                        model: [
                            { name: "obsidian",  label: "Obsidian" },
                            { name: "emerald",   label: "Emerald" },
                            { name: "amethyst",  label: "Amethyst" },
                            { name: "onyx",      label: "Onyx" },
                            { name: "champagne", label: "Champagne" }
                        ]
                        delegate: Rectangle {
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
                            Text {
                                visible: active
                                anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 8 }
                                text: "✓"; color: Theme.accent; font { pixelSize: Globals.sp(14); bold: true }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: App.settings.themeName = modelData.name
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 10
                    Text { text: qsTr("Accent"); color: Theme.textSecondary; font.pixelSize: Globals.sp(20) }
                    Rectangle { width: 28; height: 28; radius: 6; color: Theme.accent; border.color: Theme.border; border.width: 1 }
                    Item { Layout.fillWidth: true }
                    AppButton { text: qsTr("Change"); onClicked: { accentDialog.selectedColor = Theme.accent; accentDialog.open() } }
                    AppButton { text: qsTr("Reset"); backgroundDefaultColor: "#374151"; onClicked: App.settings.accentColor = "" }
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
                    AppButton { text: qsTr("Clear"); backgroundDefaultColor: "#374151"; onClicked: { proxyField.text = ""; App.settings.proxy = "" } }
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
                    AppButton { text: qsTr("mpv folder"); backgroundDefaultColor: "#374151"; onClicked: Qt.openUrlExternally("file:///" + App.settings.appDir + "/mpv") }
                    AppButton { text: qsTr("mpv.conf"); backgroundDefaultColor: "#374151"; onClicked: Qt.openUrlExternally("file:///" + App.settings.appDir + "/mpv/mpv.conf") }
                    AppButton { text: qsTr("settings.ini"); backgroundDefaultColor: "#374151"; onClicked: Qt.openUrlExternally(App.settings.path) }
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