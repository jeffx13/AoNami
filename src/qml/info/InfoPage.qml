import QtQuick 2.15
import QtQuick.Controls 2.15
import "./../components"
import QtQuick.Layouts 1.15

Item {
    id:infoPage
    LoadingScreen {
        id:loadingScreen
        z:10
        anchors.centerIn: parent
        loading: infoPage.visible && app.playlist.isLoading
    }
    focus: true
    property real aspectRatio: root.width/root.height
    property real labelFontSize: 24 * (root.maximised ? 1.6 : 1)
    property var currentShow: app.currentShow


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
                    app.currentShow.episodeList.reversed = !app.currentShow.episodeList.reversed
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
        source: currentShow.coverUrl
        onStatusChanged: if (posterImage.status === Image.Error) source = "qrc:/resources/images/error_image.png"

        anchors{
            top: parent.top
            left: parent.left
        }
        width: parent.width * 0.2
        height: parent.height * 0.5
        fillMode: Image.PreserveAspectFit
    }


    Text {
        id: titleText
        text: currentShow.title.toUpperCase()
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
            onClicked:  Qt.openUrlExternally(`https://anilist.co/search/anime?search=${encodeURIComponent(titleText.text)}`);
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
            text: currentShow.description
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

        currentIndex: app.currentShow.listType + 1
        Component.onCompleted: {
            if (app.currentShow.listType !== -1)
                listTypeModel.set(0, {text: "Remove from Library"})
            else
                listTypeModel.set(0, {text: "Add to Library"})
        }

        fontSize: 20
        onActivated: (index) => {
                         if (index === 0) {
                             app.removeCurrentShowFromLibrary()
                             listTypeModel.set(0, {text: "Add to Library"})
                         } else {
                             app.addCurrentShowToLibrary(index - 1)
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
        text: app.currentShow.episodeList.continueText
        onClicked: app.continueWatching()
        visible: app.currentShow.episodeList.continueText.length !== 0
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

        Text {
            id:scoresText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            text: `<b>SCORE:</b> <font size="-0.5">${currentShow.rating}</font>`
            font.pixelSize: labelFontSize
            Layout.preferredHeight: implicitHeight
            Layout.fillWidth: true
            color: "white"
            visible: text.length !== 0
        }

        Text {
            id:statusText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            text: `<b>STATUS:</b> <font size="-0.5">${currentShow.status}</font>`
            font.pixelSize: labelFontSize
            color: "white"
            visible: text.length !== 0
            // Layout.preferredWidth: 2
            Layout.preferredHeight: implicitHeight
            //Layout.fillHeight: true
            Layout.fillWidth: true

        }
        Text {
            id:viewsText
            Layout.alignment: Qt.AlignTop | Qt.AlignCenter
            text: `<b>VIEWS:</b> <font size="-0.5">${currentShow.views}</font>`
            font.pixelSize: labelFontSize

            color: "white"


            // Layout.preferredWidth: 2
            Layout.preferredHeight: implicitHeight
            //Layout.fillHeight: true
            Layout.fillWidth: true
            visible: text.length !== 0
        }

        Text {
            id:dateAiredText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            text: `<b>DATE AIRED:</b> <font size="-0.5">${currentShow.releaseDate}</font>`
            font.pixelSize: labelFontSize
            color: "white"


            Layout.preferredHeight: implicitHeight
            //Layout.fillHeight: true
            Layout.fillWidth: true
            visible: text.length !== 0
        }
        Text {
            id:updateTimeText
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            text: `<b>UPDATE TIME:</b> <font size="-0.5">${currentShow.updateTime}</font>`
            font.pixelSize: labelFontSize
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
                text: currentShow.genresString
                font.pixelSize: 23.5 * (root.maximised ? 1.6 : 1)
                color: "white"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: text.length !== 0

            }
        }
    }


    Keys.enabled: true
    Keys.onPressed: (event) => {
                        switch (event.key){
                            case Qt.Key_Space:
                            app.continueWatching();
                            break
                            case Qt.Key_Escape:
                            infoPage.forceActiveFocus()
                            break
                        }
                    }

    Component.onCompleted: infoPage.forceActiveFocus()


}

