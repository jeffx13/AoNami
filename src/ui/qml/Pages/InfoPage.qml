import QtQuick
import QtQuick.Controls
import "./../Components"
import QtQuick.Layouts
import AoNami
import ".."
import Qt5Compat.GraphicalEffects

Item {
    id: infoPage
    focus: true
    readonly property var currentShow: App.showManager.currentShow

    HoverHandler {
        cursorShape: Qt.ArrowCursor
    }

    component MetaChip: Rectangle {
        property string iconChar
        property string chipValue
        property color  iconColor:  Theme.textSecondary
        property color  valueColor: Theme.textSecondary

        visible: chipValue.length > 0
        implicitWidth: chipRow.implicitWidth + 16
        height: 30
        radius: 15
        color: Theme.surfaceAlt
        border.color: Theme.border
        border.width: 1

        Row {
            id: chipRow
            anchors.centerIn: parent
            spacing: 4
            Text { text: iconChar;  color: iconColor;  font.pixelSize: Globals.sp(20) }
            Text { text: chipValue; color: valueColor; font.pixelSize: Globals.sp(20) }
        }
    }

    component DateChip: Rectangle {
        property string iconChar
        property string chipValue

        visible: chipValue.length > 0
        implicitWidth: dateLabel.implicitWidth + 16
        height: 26
        radius: 6
        color: Theme.surfaceAlt
        border.color: Theme.border
        border.width: 1

        Text {
            id: dateLabel
            anchors.centerIn: parent
            text: iconChar + " " + chipValue
            color: Theme.textMuted
            font.pixelSize: Globals.sp(20)
        }
    }

    function correctIndex(index) {
        return App.showManager.episodeListModel.sourceIndex(index)
    }

    Connections {
        target: App.showManager
        function onShowChanged() { epFilterField.text = ""; libraryComboBox.rebuildModel() }
    }
    Connections {
        target: App.library
        function onLibraryChanged() { libraryComboBox.rebuildModel() }
    }

    Image {
        id: bgImage
        anchors.fill: parent
        source: currentShow.coverUrl
        fillMode: Image.PreserveAspectCrop
        visible: false
    }
    FastBlur {
        anchors.fill: parent
        source: bgImage
        radius: 80
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.alpha(Theme.background, 0.82) }
            GradientStop { position: 0.4; color: Qt.alpha(Theme.background, 0.94) }
            GradientStop { position: 1.0; color: Theme.background }
        }
    }

    Rectangle {
        id: episodePanel
        width: Math.min(parent.width * 0.32, 420)
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
            topMargin: 12
            rightMargin: 12
            bottomMargin: 12
        }
        radius: 16
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Layout.margins: 8
                Layout.bottomMargin: 0
                spacing: 6

                Text {
                    text: "EPISODES"
                    color: Theme.textMuted
                    Layout.fillWidth: true
                    font {
                        pixelSize: Globals.sp(20)
                        bold: true
                        letterSpacing: 1.5
                    }
                }

                Rectangle {
                    visible: episodeListView.count > 0
                    Layout.preferredWidth: epCountText.implicitWidth + 14
                    Layout.preferredHeight: 22
                    radius: 11
                    color: Theme.accent
                    Text {
                        id: epCountText
                        anchors.centerIn: parent
                        text: episodeListView.count
                        color: "white"
                        font {
                            pixelSize: Globals.sp(20)
                            bold: true
                        }
                    }
                }

                AbstractButton {
                    id: sortBtn
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    focusPolicy: Qt.NoFocus
                    onClicked: App.showManager.episodeListModel.reversed = !App.showManager.episodeListModel.reversed
                    background: Rectangle {
                        radius: 6
                        color: parent.hovered ? Theme.border : "transparent"
                    }
                    contentItem: Item {
                        AppIcon {
                            anchors.centerIn: parent
                            name: "arrow-up-down"
                            size: 18
                            color: sortBtn.hovered ? Theme.textAccent : Theme.textMuted
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                color: Theme.border
            }

            AppTextField {
                id: epFilterField
                Layout.fillWidth: true
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.topMargin: 2
                placeholderText: qsTr("Filter episodes...")
                color: Theme.textPrimary
                placeholderTextColor: Theme.textMuted
                fontSize: 20
                onTextChanged: App.showManager.episodeListModel.filterText = text
            }

            ListView {
                id: episodeListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 4
                clip: true
                model: App.showManager.episodeListModel
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds

                property int lastWatchedIndex: App.showManager.episodeListModel.visibleIndex(App.showManager.lastWatchedIndex)

                Component.onCompleted: {
                    if (lastWatchedIndex >= 0)
                        positionViewAtIndex(lastWatchedIndex, ListView.Center)
                }

                ScrollBar.vertical: ScrollBar {
                    id: epScroll
                    policy: ScrollBar.AsNeeded
                    width: 9
                    minimumSize: 0.06
                    contentItem: Rectangle {
                        radius: 4
                        color: Theme.accent
                        opacity: epScroll.pressed ? 0.9 : (epScroll.hovered ? 0.7 : 0.45)
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }
                    background: Rectangle { radius: 4; color: Theme.surfaceAlt }
                }

                delegate: Rectangle {
                    id: ep
                    required property string title
                    required property real episodeNumber
                    required property int seasonNumber
                    required property int index
                    property bool isCurrent: episodeListView.lastWatchedIndex === index
                    property bool hovered: false

                    width: episodeListView.width
                    height: 52
                    radius: 10
                    color: isCurrent ? Qt.alpha(Theme.accent, 0.14) : (hovered ? Qt.alpha(Theme.accent, 0.08) : "transparent")
                    border.color: isCurrent ? Qt.alpha(Theme.accent, 0.35) : "transparent"
                    border.width: isCurrent ? 1 : 0
                    Behavior on color { ColorAnimation { duration: 80 } }

                    Rectangle {
                        visible: ep.isCurrent
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

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.AllButtons
                        onEntered: ep.hovered = true
                        onExited: ep.hovered = false
                        onClicked: (mouse) => {
                            let ci = correctIndex(index)
                            App.showManager.lastWatchedIndex = ci
                            App.playFromEpisodeList(ci, mouse.button === Qt.RightButton)
                        }
                    }

                    RowLayout {
                        anchors {
                            fill: parent
                            leftMargin: 8
                            rightMargin: 6
                        }
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: Math.max(40, epNumText.implicitWidth + 14)
                            Layout.preferredHeight: 40
                            radius: 10
                            color: ep.isCurrent ? Theme.accent : Theme.surfaceDeep
                            border.color: ep.isCurrent ? Theme.accentLight : Theme.border
                            border.width: 1

                            Text {
                                id: epNumText
                                anchors.centerIn: parent
                                text: {
                                    let n = episodeNumber
                                    return Math.floor(n) === n ? Math.floor(n).toString() : n.toFixed(1)
                                }
                                color: ep.isCurrent ? "white" : Theme.textMuted
                                font {
                                    pixelSize: Globals.sp(20)
                                    bold: true
                                }
                            }

                            Rectangle {
                                visible: ep.isCurrent
                                anchors.fill: parent
                                radius: parent.radius
                                color: "transparent"
                                border.color: Theme.accent
                                border.width: 1
                                opacity: 0

                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    running: ep.isCurrent
                                    NumberAnimation { to: 0.4; duration: 1000; easing.type: Easing.OutCubic }
                                    NumberAnimation { to: 0.0; duration: 1000; easing.type: Easing.InCubic }
                                }
                                NumberAnimation on scale {
                                    loops: Animation.Infinite
                                    running: ep.isCurrent
                                    from: 1.0
                                    to: 1.3
                                    duration: 2000
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Text {
                                text: {
                                    let s = seasonNumber > 0 ? "S" + seasonNumber.toString().padStart(2, '0') + " " : ""
                                    let n = episodeNumber
                                    let isInt = Math.floor(n) === n
                                    return s + (isInt ? "E" + Math.floor(n).toString().padStart(2, '0') : "E" + n.toFixed(1))
                                }
                                font {
                                    pixelSize: Globals.sp(20)
                                    bold: true
                                }
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                color: ep.isCurrent ? Theme.textAccent : Theme.textSecondary
                            }

                            MarqueeText {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                text: title
                                fontSize: 20
                                color: ep.isCurrent ? Theme.accentLight : Theme.textMuted
                                spacing: 30
                                marqueeSpeed: 50
                            }
                        }

                        AbstractButton {
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 32
                            visible: !ep.isCurrent
                            focusPolicy: Qt.NoFocus
                            onClicked: {
                                App.showManager.lastWatchedIndex = correctIndex(index)
                                App.library.updateProgress(currentShow.link, correctIndex(index), 0, true)
                            }
                            background: Rectangle {
                                radius: 8
                                color: parent.hovered ? Theme.border : "transparent"
                            }
                            contentItem: Item {
                                AppIcon {
                                    anchors.centerIn: parent
                                    name: "tv"
                                    size: 18
                                    color: Theme.textSecondary
                                }
                            }
                        }

                        AbstractButton {
                            id: dlBtn
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 32
                            focusPolicy: Qt.NoFocus
                            property bool downloaded: false
                            onClicked: {
                                downloaded = true
                                enabled = false
                                App.downloadCurrentShow(correctIndex(ep.index), correctIndex(ep.index))
                            }
                            background: Rectangle {
                                radius: 8
                                color: dlBtn.downloaded ? Qt.alpha(Theme.success, 0.15) : (dlBtn.hovered ? Theme.border : "transparent")
                            }
                            contentItem: Item {
                                AppIcon {
                                    anchors.centerIn: parent
                                    name: dlBtn.downloaded ? "check" : "download"
                                    size: 18
                                    color: dlBtn.downloaded ? Theme.success : Theme.textMuted
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Flickable {
        id: infoFlickable
        anchors {
            top: parent.top
            left: parent.left
            right: episodePanel.left
            bottom: parent.bottom
            topMargin: 12
            leftMargin: 12
            rightMargin: 10
            bottomMargin: 12
        }
        contentHeight: infoCol.implicitHeight + 24
        contentWidth: width
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: infoCol
            width: infoFlickable.width
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                Rectangle {
                    Layout.preferredWidth: Math.min(infoFlickable.width * 0.28, 220)
                    Layout.preferredHeight: Layout.preferredWidth * 1.42
                    radius: 14
                    color: Theme.surfaceDeep
                    clip: true
                    border.color: Theme.border
                    border.width: 1

                    Image {
                        id: poster
                        anchors {
                            fill: parent
                            margins: 2
                        }
                        source: currentShow.coverUrl
                        fillMode: Image.PreserveAspectCrop
                        onStatusChanged: {
                            if (status === Image.Error)
                                source = "qrc:/AoNami/resources/images/error_image.png"
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: posterHover.containsMouse ? Theme.border : "transparent"
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    MouseArea {
                        id: posterHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: coverPopup.open()
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: currentShow.title
                        font {
                            pixelSize: Globals.sp(28)
                            bold: true
                        }
                        color: Theme.textPrimary
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            cursorShape: Qt.PointingHandCursor
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.LeftButton) {
                                    Globals.lastSearch = currentShow.title
                                    App.explore(currentShow.title, 1, false)
                                    Globals.gotoPage(0)
                                } else {
                                    App.copyToClipboard(currentShow.title)
                                }
                            }
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            visible: (currentShow.status ?? "").length > 0
                            width: statusText.implicitWidth + 20
                            height: 30
                            radius: 15
                            color: {
                                let s = (currentShow.status ?? "").toLowerCase()
                                if (s.includes("air") || s.includes("ongoing")) return Theme.success
                                if (s.includes("finish") || s.includes("complete")) return Theme.accent
                                return Theme.textMuted
                            }
                            Text {
                                id: statusText
                                anchors.centerIn: parent
                                text: currentShow.status ?? ""
                                color: "white"
                                font {
                                    pixelSize: Globals.sp(20)
                                    bold: true
                                }
                            }
                        }

                        MetaChip {
                            iconChar:   "\u2605"
                            chipValue:  currentShow.rating ?? ""
                            iconColor:  "#FBBF24"
                            valueColor: Theme.textPrimary
                        }

                        MetaChip {
                            iconChar:  "\uD83D\uDC41"
                            chipValue: currentShow.views ?? ""
                        }

                        Rectangle {
                            visible: currentShow.provider?.name?.length > 0
                            width: provText.implicitWidth + 16
                            height: 30
                            radius: 15
                            color: Theme.surfaceAlt
                            border.color: Theme.border
                            border.width: 1
                            Text {
                                id: provText
                                anchors.centerIn: parent
                                text: currentShow.provider?.name ?? ""
                                color: Theme.textAccent
                                font.pixelSize: Globals.sp(20)
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally(currentShow.provider?.hostUrl ?? '#')
                            }
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 6
                        DateChip { iconChar: "\uD83D\uDCC5"; chipValue: currentShow.releaseDate ?? "" }
                        DateChip { iconChar: "\uD83D\uDD04"; chipValue: currentShow.updateTime  ?? "" }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        AppButton {
                            visible: App.showManager.continueText.length > 0
                            text: ""
                            radius: height / 2
                            Layout.preferredHeight: 42
                            Layout.preferredWidth: Math.min(260, infoFlickable.width * 0.45)
                            leftPadding: 0
                            rightPadding: 0
                            backgroundDefaultColor: Theme.accent
                            contentItem: Item {
                                anchors {
                                    fill: parent
                                    leftMargin: 16
                                    rightMargin: 16
                                }
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 8
                                    AppIcon {
                                        name: "play"
                                        size: 18
                                        color: "white"
                                    }
                                    MarqueeText {
                                        Layout.fillWidth: true
                                        color: "white"
                                        text: App.showManager.continueText
                                        fontSize: 20
                                        spacing: 30
                                        marqueeSpeed: 50
                                    }
                                }
                            }
                            onClicked: App.continueWatching()
                        }

                        AppComboBox {
                            id: libraryComboBox
                            Layout.preferredHeight: 42
                            Layout.preferredWidth: Math.min(180, infoFlickable.width * 0.28)
                            focus: false
                            activeFocusOnTab: false
                            placeholderText: "+ Library"
                            currentIndex: -1

                            function rebuildModel() {
                                libraryTypeModel.clear()
                                const types = Globals.libraryTypes
                                const lt = App.library.getLibraryType(currentShow.link)
                                if (lt === -1) {
                                    for (let i = 0; i < types.length; i++)
                                        libraryTypeModel.append({ text: types[i], disabled: false })
                                    placeholderText = "+ Library"
                                    currentIndex = -1
                                } else {
                                    libraryTypeModel.append({ text: "Remove", disabled: false })
                                    for (let i = 0; i < types.length; i++)
                                        libraryTypeModel.append({ text: types[i], disabled: i === lt })
                                    placeholderText = ""
                                    currentIndex = lt + 1
                                }
                            }

                            Component.onCompleted: rebuildModel()

                            onActivated: (index) => {
                                const lt = App.library.getLibraryType(currentShow.link)
                                if (lt === -1) {
                                    App.addToLibrary(-1, index)
                                } else if (index === 0) {
                                    App.library.remove(currentShow.link)
                                } else {
                                    let t = index - 1
                                    if (t !== lt) App.addToLibrary(-1, t)
                                }
                                rebuildModel()
                            }

                            model: ListModel { id: libraryTypeModel }
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            Flow {
                visible: (currentShow.genresString ?? "").length > 0
                Layout.fillWidth: true
                spacing: 6

                Repeater {
                    model: (currentShow.genresString ?? "").split(",").map(s => s.trim()).filter(s => s.length > 0)
                    delegate: Rectangle {
                        required property string modelData
                        width: chipText.implicitWidth + 20
                        height: 32
                        radius: 16
                        color: chipMa.containsMouse ? Qt.alpha(Theme.accent, 0.18) : Theme.surfaceAlt
                        border.color: chipMa.containsMouse ? Theme.accent : Theme.border
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }

                        Text {
                            id: chipText
                            anchors.centerIn: parent
                            text: modelData
                            color: chipMa.containsMouse ? Theme.textAccent : Theme.textMuted
                            font.pixelSize: Globals.sp(20)
                            Behavior on color { ColorAnimation { duration: 120 } }
                        }

                        MouseArea {
                            id: chipMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                Globals.lastSearch = modelData
                                App.explore(modelData, 1, false)
                                Globals.gotoPage(0)
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: descCol.implicitHeight + 24
                radius: 14
                color: Theme.surface
                border.color: Theme.border
                border.width: 1

                ColumnLayout {
                    id: descCol
                    anchors {
                        fill: parent
                        margins: 12
                    }
                    spacing: 8

                    Text {
                        text: "SYNOPSIS"
                        color: Theme.textMuted
                        font {
                            pixelSize: Globals.sp(20)
                            bold: true
                            letterSpacing: 1.5
                        }
                    }

                    TextEdit {
                        Layout.fillWidth: true
                        text: currentShow.description.length > 0 ? currentShow.description : "No Description"
                        readOnly: true
                        selectByMouse: true
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.Wrap
                        color: "#94A3B8"
                        font.pixelSize: Globals.sp(20)
                        onLinkActivated: (link) => Qt.openUrlExternally(link)
                        HoverHandler {
                            enabled: parent.hoveredLink.length > 0
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    text: "LINKS"
                    color: Theme.textMuted
                    font {
                        pixelSize: Globals.sp(20)
                        bold: true
                    }
                }

                Repeater {
                    model: [
                        { icon: "https://anilist.co/img/icons/android-chrome-192x192.png",
                          url: "https://anilist.co/search/anime?search=" },
                        { icon: "https://myanimelist.net/img/common/pwa/launcher-icon-3x.png",
                          url: "https://myanimelist.net/search/all?q=" },
                        { icon: "https://m.media-amazon.com/images/G/01/imdb/images-ANDW73HA/favicon_desktop_32x32._CB1582158068_.png",
                          url: "https://www.imdb.com/find?q=" },
                        { icon: "https://cdn-icons-png.flaticon.com/512/3670/3670356.png",
                          url: "https://movie.douban.com/subject_search?search_text=" }
                    ]
                    ImageButton {
                        required property var modelData
                        source: modelData.icon
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        onClicked: Qt.openUrlExternally(modelData.url + encodeURIComponent(currentShow.title))
                    }
                }

                Item { Layout.fillWidth: true }
            }

            RowLayout {
                visible: episodeListView.count > 0
                Layout.fillWidth: true
                spacing: 8

                AppIcon {
                    name: "download"
                    size: 18
                    color: Theme.textMuted
                }

                AppSpinBox {
                    id: startSpinBox
                    Layout.preferredWidth: 110
                    value: App.showManager.lastWatchedIndex + 1
                    from: 1
                    to: episodeListView.count
                    onValueModified: {
                        if (value > endSpinBox.value)
                            endSpinBox.value = value
                    }
                    onToChanged: value = App.showManager.lastWatchedIndex + 1
                }

                AppIcon {
                    name: "arrow-right"
                    size: 18
                    color: Theme.textMuted
                }

                AppSpinBox {
                    id: endSpinBox
                    Layout.preferredWidth: 110
                    value: episodeListView.count
                    from: 1
                    to: episodeListView.count
                    onValueModified: {
                        if (value < startSpinBox.value)
                            startSpinBox.value = value
                    }
                }

                AppButton {
                    text: "Download"
                    Layout.preferredHeight: 38
                    onClicked: App.downloadCurrentShow(startSpinBox.value - 1, endSpinBox.value - 1)
                }

                Item { Layout.fillWidth: true }
            }

            Item { height: 12 }
        }
    }

    Popup {
        id: coverPopup
        modal: true
        focus: true
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: parent
        width: Globals.appWidth > 0 ? Math.min(Globals.appWidth * 0.6, 720) : 720
        height: Globals.appHeight > 0 ? Math.min(Globals.appHeight * 0.8, 960) : 960
        Overlay.modal: Rectangle { color: "#00000099" }
        background: Rectangle {
            color: Theme.surfaceDeep
            radius: 14
            border.color: Theme.border
            border.width: 1
            clip: true
        }
        contentItem: Image {
            anchors.fill: parent
            source: currentShow.coverUrl
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }
    }

    Keys.enabled: true
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Space)
            App.continueWatching()
        else if (event.key === Qt.Key_Escape)
            infoPage.forceActiveFocus()
    }

    Component.onCompleted: infoPage.forceActiveFocus()
}