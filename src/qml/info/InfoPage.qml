import QtQuick 2.15
import QtQuick.Controls 2.15
import "./../components"
import QtQuick.Layouts 1.15
import Kyokou.App.Main
Item {
    id:infoPage
    focus: true
    property real aspectRatio: root.width/root.height
    property real labelFontSize: 24 * (root.maximised ? 1.6 : 1)
    property var currentShow: App.currentShow

    Rectangle{
        id:episodeListHeader
        height: 30
        color: "grey"
        RowLayout {
            anchors.fill: parent
            Text {
                id: countLabel
                Layout.alignment: Qt.AlignTop
                Layout.fillHeight: true
                Layout.fillWidth: true
                text: episodeList.count + " Episodes"
                font.bold: true
                color: "white"
                font.pixelSize: 25
                visible: episodeList.count > 0
            }
            ImageButton {
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.preferredWidth: height
                Layout.rightMargin: 10
                id:reverseButton
                source: "qrc:/resources/images/sorting-arrows.png"

                onClicked: {
                    App.currentShow.episodeList.reversed = !App.currentShow.episodeList.reversed
                }
            }
        }
        anchors{
            right:parent.right
            top:parent.top
        }
        width: parent.width * 0.3
    }

    EpisodeListView {
        id: episodeList
        Layout.fillHeight: true
        Layout.fillWidth: true
        anchors{
            right:parent.right
            top:episodeListHeader.bottom
            bottom: parent.bottom
        }
        width: episodeListHeader.width
    }

    Image {
        id: posterImage
        source: App.currentShow.coverUrl
        onStatusChanged: if (posterImage.status === Image.Error) source = "qrc:/resources/images/error_image.png"

        anchors {
            top: parent.top
            left: parent.left
        }
        width: parent.width * 0.2
        height: parent.height * 0.5
        fillMode: Image.PreserveAspectFit
    }


    Text {
        id: titleText
        text: App.currentShow.title.toUpperCase()
        font.bold: true
        color: "white"
        wrapMode: Text.Wrap
        font.pixelSize: 26 * (root.maximised ? 1.6 : 1)
        height: contentHeight
        anchors {
            top: parent.top
            left:posterImage.right
            right: episodeList.left
        }
        MouseArea{
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
            onClicked: function (mouse) {
                if (mouse.button === Qt.LeftButton) {
                    root.searchShow(App.currentShow.title)

                } else if (mouse.button === Qt.MiddleButton) {

                } else {
                    App.copyToClipboard(App.currentShow.title)
                }
            }
            cursorShape: Qt.PointingHandCursor
        }


    }

    Flickable{
        id:descriptionBox
        interactive: true
        boundsBehavior: Flickable.StopAtBounds
        contentHeight: descriptionLabel.contentHeight + 10
        clip: true
        anchors{
            left: posterImage.right
            right: episodeList.left
            top: titleText.bottom
            bottom: continueWatchingButton.top
            bottomMargin: 5
            leftMargin: 2
            rightMargin: 2
            topMargin: 2
        }

        Text {
            id:descriptionLabel
            text: App.currentShow.description.length > 0 ? App.currentShow.description : "No Description"
            anchors.fill: parent
            height: contentHeight
            color: "white"
            wrapMode: Text.Wrap
            font.pixelSize: 24 * (root.maximised ? 1.6 : 1)
        }
    }

    CustomComboBox {
        id:libraryComboBox
        anchors {
            top: posterImage.bottom
            left: parent.left
            right: posterImage.right
            topMargin: 5
        }
        focus: false
        activeFocusOnTab: false
        height: parent.height * 0.07

        currentIndex: App.currentShow.listType + 1
        Component.onCompleted: {
            if (App.currentShow.listType !== -1)
                listTypeModel.set(0, {text: "Remove from Library"})
            else
                listTypeModel.set(0, {text: "Add to Library"})
        }

        fontSize: 20
        onActivated: (index) => {
                         if (index === 0) {
                             App.removeCurrentShowFromLibrary()
                             listTypeModel.set(0, {text: "Add to Library"})
                         } else {
                             App.addCurrentShowToLibrary(index - 1)
                             listTypeModel.set(0, {text: "Remove from Library"})
                         }
                     }

        model: ListModel{
            id: listTypeModel
            ListElement { text: "" }
            ListElement { text: "Watching" }
            ListElement { text: "Planned" }
            ListElement { text: "On Hold" }
            ListElement { text: "Dropped" }
            ListElement { text: "Completed" }
        }
    }

    CustomButton {
        id:continueWatchingButton
        Layout.alignment: Qt.AlignTop | Qt.AlignLeft
        text: App.currentShow.continueText
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
            right: episodeList.left
        }
        RowLayout {
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Text {
                id:providerNameText
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                text: `<b>Provider:</b> <font size="-0.5">${App.currentShow.provider ? App.currentShow.provider.name : ""}</font>`
                font.pixelSize: infoPage.labelFontSize
                Layout.preferredHeight: implicitHeight
                Layout.preferredWidth: 5
                Layout.fillWidth: true
                color: "white"
                visible: text.length !== 0
                MouseArea{
                    anchors.fill: parent
                    onClicked:  {
                        if (App.currentShow.provider)
                        Qt.openUrlExternally(App.currentShow.provider.hostUrl);
                    }
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Text {
                id:statusText
                text: `<b>STATUS:</b> <font size="-0.5">${App.currentShow.status}</font>`
                font.pixelSize: infoPage.labelFontSize
                color: "white"
                visible: text.length !== 0
                Layout.preferredHeight: implicitHeight
                Layout.fillWidth: true
                Layout.preferredWidth: 5

            }
        }

        Text {
            id:scoresText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            text: `<b>SCORE:</b> <font size="-0.5">${App.currentShow.rating}</font>`
            font.pixelSize: infoPage.labelFontSize
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            color: "white"
            visible: text.length !== 0
        }


        Text {
            id:viewsText
            Layout.alignment: Qt.AlignTop | Qt.AlignCenter
            text: `<b>VIEWS:</b> <font size="-0.5">${App.currentShow.views}</font>`
            font.pixelSize: infoPage.labelFontSize

            color: "white"
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            visible: viewsText.text.length !== 0
        }

        Text {
            id:dateAiredText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            text: `<b>DATE AIRED:</b> <font size="-0.5">${App.currentShow.releaseDate}</font>`
            font.pixelSize: infoPage.labelFontSize
            color: "white"

            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            visible: text.length !== 0
        }
        Text {
            id:updateTimeText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            text: `<b>UPDATE TIME:</b> <font size="-0.5">${App.currentShow.updateTime}</font>`
            font.pixelSize: infoPage.labelFontSize
            color: "white"
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
                color: "white"
                Layout.fillHeight: true
            }
            Text {
                text: App.currentShow.genresString
                font.pixelSize: 21 * (root.maximised ? 1.6 : 1)
                color: "white"
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
                color: "white"
                Layout.fillHeight: true
            }
            ImageButton {
                source: "https://anilist.co/img/icons/android-chrome-192x192.png"
                Layout.fillHeight: true
                Layout.maximumWidth: 32
                Layout.maximumHeight: 32
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                onClicked: Qt.openUrlExternally(`https://anilist.co/search/anime?search=${encodeURIComponent(App.currentShow.title)}`);
            }
            ImageButton {
                source: "https://myanimelist.net/img/common/pwa/launcher-icon-3x.png"
                Layout.fillHeight: true
                Layout.maximumWidth: 32
                Layout.maximumHeight: 32
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                onClicked: Qt.openUrlExternally(`https://myanimelist.net/search/all?q=${encodeURIComponent(App.currentShow.title)}`);
            }
            ImageButton {
                source: "https://m.media-amazon.com/images/G/01/imdb/images-ANDW73HA/favicon_desktop_32x32._CB1582158068_.png"
                Layout.fillHeight: true
                Layout.maximumWidth: 32
                Layout.maximumHeight: 32
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                onClicked: Qt.openUrlExternally(`https://www.imdb.com/find?q=${encodeURIComponent(App.currentShow.title)}`);

            }
            ImageButton {
                source: "https://img1.doubanio.com/favicon.ico"
                Layout.fillHeight: true
                Layout.maximumWidth: 32
                Layout.maximumHeight: 32
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                onClicked: Qt.openUrlExternally(`https://movie.douban.com/subject_search?search_text=${encodeURIComponent(App.currentShow.title)}`);
            }

        }

        RowLayout {
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            visible: episodeList.count > 0
            SpinBox {
                id: startSpinBox
                value: App.currentShow.lastWatchedIndex + 1
                from : 1
                to: episodeList.count
                onValueModified: if (startSpinBox.value > endSpinBox.value) {
                                     endSpinBox.value = startSpinBox.value
                                 }
                onToChanged: {
                    startSpinBox.value = App.currentShow.lastWatchedIndex + 1
                }

                editable:true
                Layout.fillWidth: true
                Layout.preferredWidth: 2
            }
            SpinBox {
                id: endSpinBox
                value: episodeList.count
                from : 1
                to: episodeList.count
                onValueModified: if (endSpinBox.value < startSpinBox.value) {
                                     startSpinBox.value = endSpinBox.value
                                 }
                editable:true
                Layout.fillWidth: true
                Layout.preferredWidth: 2
            }
            CustomButton {
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

