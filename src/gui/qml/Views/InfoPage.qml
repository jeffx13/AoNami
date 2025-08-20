import QtQuick
import QtQuick.Controls
import "./../Components"
import QtQuick.Layouts
import Kyokou.App.Main

Item {
    id: infoPage
    focus: true
    
    property real aspectRatio: root.width / root.height
    property real labelFontSize: (24 * (root.maximised ? 1.6 : 1) * 0.75) + 2
    property var currentShow: App.showManager.currentShow
    
    function correctIndex(index) { 
        return App.showManager.episodeListModel.reversed ? episodeListView.count - 1 - index : index 
    }

    // Episode List Header
    Rectangle {
        id: episodeListHeader
        height: 40
        color: "#0f1324cc"
        radius: 10
        border.color: "#334E5BF2"
        border.width: 1
        width: parent.width * 0.3
        
        anchors {
            right: parent.right
            top: parent.top
            rightMargin: 12
            topMargin: 12
        }
        
        RowLayout {
            anchors.fill: parent
            
            Text {
                id: countLabel
                Layout.alignment: Qt.AlignVCenter
                Layout.fillHeight: true
                Layout.fillWidth: true
                text: "Total Episodes: " + episodeListView.count
                font.bold: true
                color: "#e8ebf6"
                font.pixelSize: 24 * root.fontSizeMultiplier
                visible: episodeListView.count > 0
            }
            
            ImageButton {
                id: reverseButton
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.preferredWidth: height
                Layout.rightMargin: 10
                source: "qrc:/resources/images/sorting-arrows.png"
                
                onClicked: {
                    App.showManager.episodeListModel.reversed = !App.showManager.episodeListModel.reversed
                }
            }
        }
    }

    // Episode List
    ListView {
        id: episodeListView
        width: episodeListHeader.width
        clip: true
        model: App.showManager.episodeListModel
        spacing: 8
        boundsMovement: Flickable.StopAtBounds
        
        anchors {
            left: episodeListHeader.left
            right: episodeListHeader.right
            top: episodeListHeader.bottom
            bottom: parent.bottom
            bottomMargin: 12
        }

        property real lastWatchedIndex: App.showManager.episodeListModel.reversed ? 
            episodeListView.count - 1 - App.showManager.lastWatchedIndex : 
            App.showManager.lastWatchedIndex

        Component.onCompleted: {
            if (App.showManager.lastWatchedIndex !== -1) {
                episodeListView.positionViewAtIndex(lastWatchedIndex, ListView.Center)
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
        
        delegate: Rectangle {
            id: delegateRect
            width: episodeListView.width
            height: episodeTitle.text.length > 0 ? 70 : 50
            radius: 10
            border.width: 1
            border.color: hovered ? "#334E5BF2" : "#141a2f"
            color: isCurrent ? "#1c2448" : (hovered ? "#151c36" : "#0f1324")
            
            property bool isCurrent: episodeListView.lastWatchedIndex === index
            property bool hovered: false
            
            required property string title
            required property int episodeNumber
            required property int seasonNumber
            required property int index

            Behavior on color { 
                ColorAnimation { 
                    duration: 120
                    easing.type: Easing.OutCubic 
                } 
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.AllButtons
                
                onEntered: delegateRect.hovered = true
                onExited: delegateRect.hovered = false
                onClicked: (mouse) => {
                    let correctedIndex = correctIndex(index)
                    App.showManager.lastWatchedIndex = correctedIndex
                    let appendNotPlay = mouse.button === Qt.RightButton
                    App.playFromEpisodeList(correctedIndex, appendNotPlay)
                }
            }
            
            // Current episode accent
            Rectangle {
                width: isCurrent ? 4 : 0
                color: "#4E5BF2"
                radius: width
                
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                    topMargin: 8
                    bottomMargin: 8
                }
            }
            
            // Episode info
            ColumnLayout {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    leftMargin: 10
                    rightMargin: 10
                    topMargin: 6
                    bottomMargin: 6
                }
                
                Text {
                    id: episodeNumberText
                    text: {
                        let seasonPart = seasonNumber > 0 ? `(S${seasonNumber.toString().padStart(2, '0')})` : ""
                        let episodePart = `E${episodeNumber.toString().padStart(2, '0')}`
                        return seasonPart + episodePart
                    }
                    font.pixelSize: 20 * root.fontSizeMultiplier
                    font.bold: true
                    Layout.fillWidth: true
                    Layout.preferredHeight: implicitHeight
                    elide: Text.ElideRight
                    color: "#e8ebf6"
                }

                Text {
                    id: episodeTitle
                    text: title
                    font.pixelSize: 20 * root.fontSizeMultiplier
                    Layout.fillWidth: true
                    Layout.preferredHeight: implicitHeight
                    elide: Text.ElideRight
                    wrapMode: Text.Wrap
                    color: "#cfd5e6"
                    visible: text.length > 0
                }

                Item {
                    Layout.fillHeight: true
                }
            }

            // Action buttons
            RowLayout {
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    rightMargin: 10
                }
                
                ImageButton {
                    id: setWatchedButton
                    source: "qrc:/resources/images/tv.png"
                    Layout.fillHeight: true
                    Layout.preferredWidth: 28
                    Layout.fillWidth: false
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    visible: !delegateRect.isCurrent
                    
                    onClicked: {
                        App.showManager.lastWatchedIndex = correctIndex(index)
                        App.library.updateProgress(App.showManager.currentShow.link, correctIndex(index), 0)
                    }
                }
                
                Item {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 8
                    Layout.fillWidth: false
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    visible: !setWatchedButton.visible
                }

                ImageButton {
                    source: "qrc:/resources/images/download-button.png"
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.preferredWidth: 28
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    
                    onClicked: {
                        enabled = false
                        source = "qrc:/resources/images/download_selected.png"
                        let correctedIndex = correctIndex(delegateRect.index)
                        App.downloadCurrentShow(correctedIndex, correctedIndex)
                    }
                }
            }
        }
    }

    // Poster Image
    Rectangle {
        id: posterImage
        width: parent.width * 0.2
        height: parent.height * 0.5
        radius: 12
        color: "#0f1324cc"
        border.color: "#334E5BF2"
        border.width: 1
        clip: true
        
        anchors {
            top: parent.top
            left: parent.left
            topMargin: 12
            leftMargin: 12
        }

        Image {
            id: poster
            anchors.fill: parent
            source: currentShow.coverUrl
            fillMode: Image.PreserveAspectFit
            
            onStatusChanged: {
                if (poster.status === Image.Error) {
                    source = "qrc:/resources/images/error_image.png"
                }
            }
        }
    }

    // Title
    Text {
        id: titleText
        text: currentShow.title.toUpperCase()
        font.bold: true
        color: "#e8ebf6"
        wrapMode: Text.Wrap
        font.pixelSize: 26 * (root.maximised ? 1.6 : 1)
        height: contentHeight
        
        anchors {
            top: parent.top
            left: posterImage.right
            right: episodeListView.left
            topMargin: 16
            leftMargin: 16
            rightMargin: 12
        }
        
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            
            onClicked: function (mouse) {
                if (mouse.button === Qt.LeftButton) {
                    root.lastSearch = currentShow.title
                    App.explore(currentShow.title, 1, false)
                    root.gotoPage(0)
                } else if (mouse.button === Qt.RightButton) {
                    App.copyToClipboard(currentShow.title)
                }
            }
        }
    }

    // Description Card Background
    Rectangle {
        id: descriptionCard
        radius: 12
        color: "#0f1324cc"
        border.color: "#334E5BF2"
        border.width: 1
        
        anchors {
            left: posterImage.right
            right: episodeListView.left
            top: titleText.bottom
            bottom: continueWatchingButton.top
            bottomMargin: 5
            leftMargin: 14
            rightMargin: 12
            topMargin: 12
        }
    }

    // Description Text
    Flickable {
        id: descriptionBox
        interactive: true
        boundsBehavior: Flickable.StopAtBounds
        contentHeight: descriptionLabel.contentHeight + 10
        clip: true
        
        anchors {
            left: posterImage.right
            right: episodeListView.left
            top: titleText.bottom
            bottom: continueWatchingButton.top
            bottomMargin: 5
            leftMargin: 16
            rightMargin: 16
            topMargin: 16
        }

        TextEdit {
            id: descriptionLabel
            text: currentShow.description.length > 0 ? currentShow.description : "No Description"
            anchors.fill: parent
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.Wrap
            color: "#cfd5e6"
            font.pixelSize: (24 * (root.maximised ? 1.6 : 1)) + 2
            onLinkActivated: function(link) {
                Qt.openUrlExternally(link)
            }
        }
    }

    // Library ComboBox
    AppComboBox {
        id: libraryComboBox
        height: parent.height * 0.0525
        focus: false
        activeFocusOnTab: false
        fontSize: 20
        placeholderText: "Add To Library"
        currentIndex: -1

        anchors {
            top: posterImage.bottom
            left: parent.left
            right: posterImage.right
            topMargin: 10
            leftMargin: 12
            rightMargin: 12
        }

        function rebuildModel() {
            libraryTypeModel.clear()
            const typeNames = ["Watching", "Planned", "Paused", "Dropped", "Completed"]
            const libraryType = App.library.getLibraryType(currentShow.link)
            if (libraryType === -1) {
                // Not in library: no explicit "Add" option, just types
                for (let i = 0; i < typeNames.length; i++) {
                    libraryTypeModel.append({ text: typeNames[i], disabled: false })
                }
                placeholderText = "Add To Library"
                currentIndex = -1
            } else {
                // In library: first item is Remove, and current type is disabled
                libraryTypeModel.append({ text: "Remove From Library", disabled: false })
                for (let i = 0; i < typeNames.length; i++) {
                    libraryTypeModel.append({ text: typeNames[i], disabled: i === libraryType })
                }
                placeholderText = ""
                currentIndex = libraryType + 1
            }
        }

        Component.onCompleted: rebuildModel()

        onActivated: function(index) {
            const priorType = App.library.getLibraryType(currentShow.link)
            if (priorType === -1) {
                // Add with selected type
                const selectedType = index
                App.addToLibrary(-1, selectedType)
            } else {
                if (index === 0) {
                    // Remove
                    App.library.remove(currentShow.link)
                } else {
                    const newType = index - 1
                    if (newType !== priorType) {
                        App.addToLibrary(-1, newType)
                    }
                }
            }
            rebuildModel()
        }

        model: ListModel { id: libraryTypeModel }
    }

    // Continue Watching Button
    AppButton {
        id: continueWatchingButton
        text: App.showManager.continueText
        fontSize: 15
        radius: height
        width: descriptionBox.width * 0.5
        visible: text.length !== 0
        
        anchors {
            top: libraryComboBox.top
            bottom: libraryComboBox.bottom
            horizontalCenter: descriptionBox.horizontalCenter
        }
        
        onClicked: App.continueWatching()
    }

    // Show Information
    ColumnLayout {
        anchors {
            top: libraryComboBox.bottom
            left: parent.left
            right: episodeListView.left
            topMargin: 16
            leftMargin: 12
            rightMargin: 12
        }
        
        // Provider and Status
        RowLayout {
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            
            TextEdit {
                id: providerNameText
                text: `<b>Provider:</b> <font size="-0.5">${currentShow.provider ? currentShow.provider.name : ""}</font>`
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                font.pixelSize: infoPage.labelFontSize
                color: "#e8ebf6"
                visible: currentShow.provider && currentShow.provider.name && currentShow.provider.name.length > 0
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                Layout.preferredHeight: implicitHeight
                Layout.preferredWidth: 5
                Layout.fillWidth: true
            }
            
            TextEdit {
                id: statusText
                text: `<b>STATUS:</b> <font size="-0.5">${currentShow.status}</font>`
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                font.pixelSize: infoPage.labelFontSize
                color: "#e8ebf6"
                visible: currentShow.status && currentShow.status.length > 0
                Layout.preferredHeight: implicitHeight
                Layout.fillWidth: true
                Layout.preferredWidth: 5
            }
        }

        // Score
        TextEdit {
            id: scoresText
            text: `<b>SCORE:</b> <font size="-0.5">${currentShow.rating}</font>`
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.Wrap
            font.pixelSize: infoPage.labelFontSize
            color: "#e8ebf6"
            visible: currentShow.rating !== undefined && String(currentShow.rating).length > 0
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
        }

        // Views
        TextEdit {
            id: viewsText
            text: `<b>VIEWS:</b> <font size="-0.5">${currentShow.views}</font>`
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.Wrap
            font.pixelSize: infoPage.labelFontSize
            color: "#e8ebf6"
            visible: currentShow.views !== undefined && String(currentShow.views).length > 0
            Layout.alignment: Qt.AlignTop | Qt.AlignCenter
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
        }

        // Date Aired
        TextEdit {
            id: dateAiredText
            text: `<b>DATE AIRED:</b> <font size="-0.5">${currentShow.releaseDate}</font>`
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.Wrap
            font.pixelSize: infoPage.labelFontSize
            color: "#e8ebf6"
            visible: currentShow.releaseDate && currentShow.releaseDate.length > 0
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
        }
        
        // Update Time
        TextEdit {
            id: updateTimeText
            text: `<b>UPDATE TIME:</b> <font size="-0.5">${currentShow.updateTime}</font>`
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.Wrap
            font.pixelSize: infoPage.labelFontSize
            color: "#e8ebf6"
            visible: currentShow.updateTime && currentShow.updateTime.length > 0
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
        }
        
        // Genres
        RowLayout {
            visible: currentShow.genresString && currentShow.genresString.length > 0
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            
            Text {
                text: "<b>GENRE(S):</b>"
                font.pixelSize: infoPage.labelFontSize
                color: "#e8ebf6"
                Layout.fillHeight: true
            }
            
            TextEdit {
                text: currentShow.genresString
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                font.pixelSize: (21 * (root.maximised ? 1.6 : 1) * 0.75) + 2
                color: "#cfd5e6"
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
        
        // Database Links
        RowLayout {
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            
            Text {
                text: "<b>DB:</b>"
                font.pixelSize: infoPage.labelFontSize
                color: "#e8ebf6"
                Layout.fillHeight: true
            }
            
            ImageButton {
                source: "https://anilist.co/img/icons/android-chrome-192x192.png"
                Layout.fillHeight: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.fillWidth: false
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                
                onClicked: {
                    Qt.openUrlExternally(`https://anilist.co/search/anime?search=${encodeURIComponent(currentShow.title)}`)
                }
            }
            
            ImageButton {
                source: "https://myanimelist.net/img/common/pwa/launcher-icon-3x.png"
                Layout.fillHeight: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.fillWidth: false
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                
                onClicked: {
                    Qt.openUrlExternally(`https://myanimelist.net/search/all?q=${encodeURIComponent(currentShow.title)}`)
                }
            }
            
            ImageButton {
                source: "https://m.media-amazon.com/images/G/01/imdb/images-ANDW73HA/favicon_desktop_32x32._CB1582158068_.png"
                Layout.fillHeight: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.fillWidth: false
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                
                onClicked: {
                    Qt.openUrlExternally(`https://www.imdb.com/find?q=${encodeURIComponent(currentShow.title)}`)
                }
            }
            
            ImageButton {
                source: "https://img1.doubanio.com/favicon.ico"
                Layout.fillHeight: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.fillWidth: false
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                
                onClicked: {
                    Qt.openUrlExternally(`https://movie.douban.com/subject_search?search_text=${encodeURIComponent(currentShow.title)}`)
                }
            }
        }

        // Download Controls
        RowLayout {
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            visible: episodeListView.count > 0
            
            SpinBox {
                id: startSpinBox
                value: App.showManager.lastWatchedIndex + 1
                from: 1
                to: episodeListView.count
                editable: true
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                
                onValueModified: {
                    if (startSpinBox.value > endSpinBox.value) {
                        endSpinBox.value = startSpinBox.value
                    }
                }
                
                onToChanged: {
                    startSpinBox.value = App.showManager.lastWatchedIndex + 1
                }
            }
            
            SpinBox {
                id: endSpinBox
                value: episodeListView.count
                from: 1
                to: episodeListView.count
                editable: true
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                
                onValueModified: {
                    if (endSpinBox.value < startSpinBox.value) {
                        startSpinBox.value = endSpinBox.value
                    }
                }
            }
            
            AppButton {
                text: "Download"
                Layout.fillWidth: true
                Layout.preferredWidth: 3
                
                onClicked: {
                    App.downloadCurrentShow(startSpinBox.value - 1, endSpinBox.value - 1)
                }
            }
            
            Item {
                Layout.fillWidth: true
                Layout.preferredWidth: 3
            }
        }
    }

    // Keyboard Controls
    Keys.enabled: true
    Keys.onPressed: (event) => {
        switch (event.key) {
            case Qt.Key_Space:
                App.continueWatching()
                break
            case Qt.Key_Escape:
                infoPage.forceActiveFocus()
                break
        }
    }

    Component.onCompleted: infoPage.forceActiveFocus()
}
