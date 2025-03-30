import QtQuick 2.15
import QtQuick.Layouts 1.15
import "components"
import Kyokou.App.Main
Rectangle {

    id: sideBar
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#B7BADB" }
        GradientStop { position: 0.5; color: "#96C8ED" }
        GradientStop { position: 1.0; color: "#F3B1BF" }
    }

    width: 50
    height: parent.height


    property int currentIndex: 0
    Connections{
        target: App.currentShow
        function onShowChanged(){
            sideBar.gotoPage(1)
        }
    }
    Connections{
        target: App.play
        function onAboutToPlay(){
            sideBar.gotoPage(3);
        }
    }


    property var pages: {
        0: "explorer/ExplorerPage.qml",
        1: "info/InfoPage.qml",
        2: "library/LibraryPage.qml",
        3: "player/MpvPage.qml",
        4: "download/DownloadPage.qml",
        // 5: "settings.qml"
        5: "log.qml",
    }

    function gotoPage(index, isHistory = false){
        if (fullscreen || currentIndex === index) return;
        switch(index) {
        case 1:
            if (!App.currentShow.exists) return;
            break;
        case 3:
            mpv.peak(2000)
            mpvPage.forceActiveFocus()
            mpvPage.visible = true
            stackView.visible = false
            loadingScreen.parent = mpvPage
            break;
        case 4:
            // @disable-check M126
            if (App.downloader==undefined) return;
            break;
        }
        if (index !== 3) {
            stackView.visible = true
            mpvPage.visible = false
            loadingScreen.parent = stackView
            stackView.replace(pages[index])
        }
        currentIndex = index
        if (!isHistory) {
            // remove all pages after current index of history
            history.splice(historyIndex + 1)
            history.push(index)
            historyIndex = history.length - 1
        }

    }

    ColumnLayout {
        height: sideBar.height
        spacing: 5
        ImageButton {
            image: selected ? "qrc:/resources/images/search_selected.png" :"qrc:/resources/images/search.png"
            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width
            onClicked: {
                sideBar.gotoPage(0)
            }
            selected: sideBar.currentIndex === 0
        }

        ImageButton {
            id: detailsPageButton
            enabled: App.currentShow.exists
            image: selected ? "qrc:/resources/images/details_selected.png" : "qrc:/resources/images/details.png"
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width
            onClicked: sideBar.gotoPage(1)
            selected: sideBar.currentIndex == 1
        }

        ImageButton {
            id:libraryPageButton
            image: selected ? "qrc:/resources/images/library_selected.png" :"qrc:/resources/images/library.png"
            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width

            onClicked: sideBar.gotoPage(2)
            selected: sideBar.currentIndex === 2
        }

        ImageButton {
            id:playerPageButton
            image: selected ? "qrc:/resources/images/tv_selected.png" :"qrc:/resources/images/tv.png"
            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width
            onClicked: sideBar.gotoPage(3)
            selected: sideBar.currentIndex === 3
        }

        ImageButton {
            id: downloadPageButton
            image: selected ? "qrc:/resources/images/download_selected.png" :"qrc:/resources/images/download.png"

            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width
            onClicked: sideBar.gotoPage(4)
            selected: sideBar.currentIndex === 4
        }
        ImageButton {
            id: logPageButton
            image: selected ? "qrc:/resources/images/download_selected.png" :"qrc:/resources/images/download.png"

            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width
            onClicked: sideBar.gotoPage(5)
            selected: sideBar.currentIndex === 5
        }

        // AnimatedImage {
        //     source: "qrc:/resources/gifs/basketball.gif"
        //     Layout.preferredWidth: sideBar.width
        //     Layout.preferredHeight: sideBar.width
        //     // MouseArea {cursorShape: Qt.PointingHandCursor}
        // }
        Rectangle {
            property string orientation: "vertical"
            color: "transparent"

            Layout.fillWidth: orientation == "horizontal"
            Layout.fillHeight: orientation == "vertical"
        }
    }
}
