import QtQuick
import QtQuick.Controls
import "./../Components"
import QtQuick.Layouts
import Kyokou.App.Main
Item {
    id:infoPage
    focus: true
    property real aspectRatio: root.width/root.height
    property real labelFontSize: 24 * (root.maximised ? 1.6 : 1)
    property var currentShow: App.showManager.currentShow

    Rectangle {
        id:episodeListHeader
        height: 40
        color: "#0f1324cc"
        radius: 10
        border.color: "#334E5BF2"
        border.width: 1
        RowLayout {
            anchors.fill: parent
            Text {
                id: countLabel
                Layout.alignment: Qt.AlignTop
                Layout.fillHeight: true
                Layout.fillWidth: true
                text: episodeListView.count + " Episodes"
                font.bold: true
                color: "#e8ebf6"
                font.pixelSize: 20
                visible: episodeListView.count > 0
            }
            ImageButton {
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.preferredWidth: height
                Layout.rightMargin: 10
                id:reverseButton
                source: "qrc:/resources/images/sorting-arrows.png"

                onClicked: {
                    App.showManager.episodeListModel.reversed = !App.showManager.episodeListModel.reversed
                }
            }
        }
        anchors{
            right:parent.right
            top:parent.top
            rightMargin: 12
            topMargin: 12
        }
        width: parent.width * 0.3
    }



    ListView {
        id: episodeListView
        Layout.fillHeight: true
        Layout.fillWidth: true
        width: episodeListHeader.width
        clip: true
        model: App.showManager.episodeListModel
        anchors{
            left: episodeListHeader.left
            right: episodeListHeader.right
            top: episodeListHeader.bottom
            bottom: parent.bottom
            bottomMargin: 12
        }

        property real lastWatchedIndex: App.showManager.episodeListModel.reversed ? episodeListView.count - 1 - App.showManager.lastWatchedIndex : App.showManager.lastWatchedIndex //TODO
        function correctIndex(index) { return App.showManager.episodeListModel.reversed ? episodeListView.count - 1 - index : index }
        Component.onCompleted: {
            if (App.showManager.lastWatchedIndex !== -1)
            episodeListView.positionViewAtIndex(lastWatchedIndex, ListView.Center)
        }

        spacing: 8
        boundsMovement: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
        delegate: Rectangle {
            id: delegateRect
            width: episodeListView.width
            height: 56
            radius: 10
            border.width: 1
            border.color: hovered ? "#334E5BF2" : "#141a2f"
            property bool isCurrent: episodeListView.lastWatchedIndex === index
            property bool hovered: false
            color: isCurrent ? "#1c2448" : (hovered ? "#151c36" : "#0f1324")
            Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

            required property string fullTitle
            required property int index

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.AllButtons;
                onEntered: delegateRect.hovered = true
                onExited: delegateRect.hovered = false
                onClicked: (mouse) => {
                    let correctedIndex = correctIndex(index)
                    App.showManager.lastWatchedIndex = correctedIndex
                    let appendNotPlay = mouse.button === Qt.RightButton
                    App.playFromEpisodeList(correctedIndex, appendNotPlay)
                }
            }
            Rectangle {
                // left accent for current episode
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
            RowLayout {
                anchors{
                    left:parent.left
                    right:parent.right
                    top:parent.top
                    bottom: parent.bottom
                    leftMargin: 10
                    rightMargin: 10
                    topMargin: 6
                    bottomMargin: 6
                }
                Text {
                    id: episodeStr
                    text:  delegateRect.fullTitle
                    font.pixelSize: 20 * root.fontSizeMultiplier
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.preferredWidth: 8
                    elide: Text.ElideRight
                    wrapMode: Text.Wrap
                    color: "#e8ebf6"
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
                        enabled = false;
                        source = "qrc:/resources/images/download_selected.png"
                        let correctedIndex = episodeListView.correctIndex(delegateRect.index)
                        App.downloadCurrentShow(correctedIndex, correctedIndex)
                    }
                }
            }
        }

    }




    Rectangle {
        id: posterImage
        anchors {
            top: parent.top
            left: parent.left
            topMargin: 12
            leftMargin: 12
        }
        width: parent.width * 0.2
        height: parent.height * 0.5
        radius: 12
        color: "#0f1324cc"
        border.color: "#334E5BF2"
        border.width: 1
        clip: true

        Image {
            id: poster
            anchors.fill: parent
            source: currentShow.coverUrl
            onStatusChanged: if (poster.status === Image.Error) source = "qrc:/resources/images/error_image.png"
            fillMode: Image.PreserveAspectFit
        }
    }


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
            left:posterImage.right
            right: episodeListView.left
            topMargin: 16
            leftMargin: 16
            rightMargin: 12
        }
        MouseArea{
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
            onClicked: function (mouse) {
                if (mouse.button === Qt.LeftButton) {
                    root.lastSearch = currentShow.title
                    App.explore(currentShow.title, 1, false)
                    root.gotoPage(0)
                } else if (mouse.button === Qt.MiddleButton) {

                } else {
                    App.copyToClipboard(currentShow.title)
                }
            }
            cursorShape: Qt.PointingHandCursor
        }


    }

    // Description card background
    Rectangle {
        id: descriptionCard
        anchors{
            left: posterImage.right
            right: episodeListView.left
            top: titleText.bottom
            bottom: continueWatchingButton.top
            bottomMargin: 5
            leftMargin: 14
            rightMargin: 12
            topMargin: 12
        }
        radius: 12
        color: "#0f1324cc"
        border.color: "#334E5BF2"
        border.width: 1
    }

    Flickable{
        id:descriptionBox
        interactive: true
        boundsBehavior: Flickable.StopAtBounds
        contentHeight: descriptionLabel.contentHeight + 10
        clip: true
        anchors{
            left: posterImage.right
            right: episodeListView.left
            top: titleText.bottom
            bottom: continueWatchingButton.top
            bottomMargin: 5
            leftMargin: 16
            rightMargin: 16
            topMargin: 16
        }

        Text {
            id:descriptionLabel
            text: currentShow.description.length > 0 ? currentShow.description : "No Description"
            anchors.fill: parent
            height: contentHeight
            color: "#cfd5e6"
            wrapMode: Text.Wrap
            font.pixelSize: 24 * (root.maximised ? 1.6 : 1)
        }
    }

    AppComboBox {
        id:libraryComboBox
        anchors {
            top: posterImage.bottom
            left: parent.left
            right: posterImage.right
            topMargin: 10
            leftMargin: 12
            rightMargin: 12
        }
        focus: false
        activeFocusOnTab: false
        height: parent.height * 0.07

        currentIndex: 0
        Component.onCompleted: {
            let libraryType = App.library.getLibraryType(currentShow.link)
            currentIndex = libraryType + 1
            if (libraryType !== -1)
                libraryTypeModel.set(0, {text: "Remove from Library"})
            else
                libraryTypeModel.set(0, {text: "Add to Library"})
        }

        fontSize: 20
        onActivated: (index) => {
                         if (index === 0) {
                             App.library.remove(currentShow.link)
                             libraryTypeModel.set(0, {text: "Add to Library"})
                         } else {
                             App.addToLibrary(-1, index - 1)
                             libraryTypeModel.set(0, {text: "Remove from Library"})
                         }
                         currentIndex = App.library.getLibraryType(currentShow.link) + 1
                     }

        model: ListModel{
            id: libraryTypeModel
            ListElement { text: "" }
            ListElement { text: "Watching" }
            ListElement { text: "Planned" }
            ListElement { text: "Paused" }
            ListElement { text: "Dropped" }
            ListElement { text: "Completed" }
        }
    }

    AppButton {
        id:continueWatchingButton
        Layout.alignment: Qt.AlignTop | Qt.AlignLeft
        text: App.showManager.continueText
        onClicked: App.continueWatching()
        visible: text.length !== 0
        fontSize: 20
        radius: height
        anchors {
            top: libraryComboBox.top
            bottom: libraryComboBox.bottom
            horizontalCenter: descriptionBox.horizontalCenter

        }
        width: descriptionBox.width * 0.5
    }


    ColumnLayout{
        anchors {
            top: libraryComboBox.bottom
            left: parent.left
            right: episodeListView.left
            topMargin: 16
            leftMargin: 12
            rightMargin: 12
        }
        RowLayout {
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Text {
                id:providerNameText
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                text: `<b>Provider:</b> <font size="-0.5">${currentShow.provider ? currentShow.provider.name : ""}</font>`
                font.pixelSize: infoPage.labelFontSize
                Layout.preferredHeight: implicitHeight
                Layout.preferredWidth: 5
                Layout.fillWidth: true
                color: "#e8ebf6"
                visible: text.length !== 0
                MouseArea{
                    anchors.fill: parent
                    onClicked:  {
                        if (currentShow.provider)
                        Qt.openUrlExternally(currentShow.provider.hostUrl);
                    }
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Text {
                id:statusText
                text: `<b>STATUS:</b> <font size="-0.5">${currentShow.status}</font>`
                font.pixelSize: infoPage.labelFontSize
                color: "#e8ebf6"
                visible: text.length !== 0
                Layout.preferredHeight: implicitHeight
                Layout.fillWidth: true
                Layout.preferredWidth: 5

            }
        }

        Text {
            id:scoresText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            text: `<b>SCORE:</b> <font size="-0.5">${currentShow.rating}</font>`
            font.pixelSize: infoPage.labelFontSize
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            color: "#e8ebf6"
            visible: text.length !== 0
        }


        Text {
            id:viewsText
            Layout.alignment: Qt.AlignTop | Qt.AlignCenter
            text: `<b>VIEWS:</b> <font size="-0.5">${currentShow.views}</font>`
            font.pixelSize: infoPage.labelFontSize

            color: "#e8ebf6"
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            visible: viewsText.text.length !== 0
        }

        Text {
            id:dateAiredText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            text: `<b>DATE AIRED:</b> <font size="-0.5">${currentShow.releaseDate}</font>`
            font.pixelSize: infoPage.labelFontSize
            color: "#e8ebf6"

            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            visible: text.length !== 0
        }
        Text {
            id:updateTimeText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            text: `<b>UPDATE TIME:</b> <font size="-0.5">${currentShow.updateTime}</font>`
            font.pixelSize: infoPage.labelFontSize
            color: "#e8ebf6"
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            visible: text.length !== 0
        }
        RowLayout {
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            // Layout.preferredHeight: implicitHeight
            Text {
                text: "<b>GENRE(S):</b>"
                font.pixelSize: 24 * (root.maximised ? 1.6 : 1)
                color: "#e8ebf6"
                Layout.fillHeight: true
            }
            Text {
                text: currentShow.genresString
                font.pixelSize: 21 * (root.maximised ? 1.6 : 1)
                color: "#cfd5e6"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: text.length !== 0

            }
        }
        RowLayout {
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            // Layout.preferredHeight: implicitHeight
            Text {
                text: "<b>DB:</b>"
                font.pixelSize: 24 * (root.maximised ? 1.6 : 1)
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
                onClicked: Qt.openUrlExternally(`https://anilist.co/search/anime?search=${encodeURIComponent(currentShow.title)}`);
            }
            ImageButton {
                source: "https://myanimelist.net/img/common/pwa/launcher-icon-3x.png"
                Layout.fillHeight: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.fillWidth: false
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                onClicked: Qt.openUrlExternally(`https://myanimelist.net/search/all?q=${encodeURIComponent(currentShow.title)}`);
            }
            ImageButton {
                source: "https://m.media-amazon.com/images/G/01/imdb/images-ANDW73HA/favicon_desktop_32x32._CB1582158068_.png"
                Layout.fillHeight: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.fillWidth: false
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                onClicked: Qt.openUrlExternally(`https://www.imdb.com/find?q=${encodeURIComponent(currentShow.title)}`);

            }
            ImageButton {
                source: "https://img1.doubanio.com/favicon.ico"
                Layout.fillHeight: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.fillWidth: false
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                onClicked: Qt.openUrlExternally(`https://movie.douban.com/subject_search?search_text=${encodeURIComponent(currentShow.title)}`);
            }

        }

        RowLayout {
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            visible: episodeListView.count > 0
            SpinBox {
                id: startSpinBox
                value: App.showManager.lastWatchedIndex + 1
                from : 1
                to: episodeListView.count
                onValueModified: if (startSpinBox.value > endSpinBox.value) {
                                     endSpinBox.value = startSpinBox.value
                                 }
                onToChanged: {
                    startSpinBox.value = App.showManager.lastWatchedIndex + 1
                }

                editable:true
                Layout.fillWidth: true
                Layout.preferredWidth: 2
            }
            SpinBox {
                id: endSpinBox
                value: episodeListView.count
                from : 1
                to: episodeListView.count
                onValueModified: if (endSpinBox.value < startSpinBox.value) {
                                     startSpinBox.value = endSpinBox.value
                                 }
                editable:true
                Layout.fillWidth: true
                Layout.preferredWidth: 2
            }
            AppButton {
                text: "Download"
                onClicked: App.downloadCurrentShow(startSpinBox.value - 1, endSpinBox.value - 1)
                Layout.fillWidth: true
                Layout.preferredWidth: 3
            }
            Item {
                Layout.fillWidth: true
                Layout.preferredWidth: 3
            }

        }
    }


    Keys.enabled: true
    Keys.onPressed: (event) => {
                        switch (event.key){
                            case Qt.Key_Space:
                            App.continueWatching();
                            break
                            case Qt.Key_Escape:
                            infoPage.forceActiveFocus()
                            break
                        }
                    }

    Component.onCompleted: infoPage.forceActiveFocus()


}

