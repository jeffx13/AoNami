import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import MpvPlayer 1.0
import Qt5Compat.GraphicalEffects
import "components"

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
        target: app.currentShow
        function onShowChanged(){
            gotoPage(1)
        }
    }
    Connections{
        target: app.play
        function onAboutToPlay(){
            gotoPage(3);
        }
    }


    property var pages: {
        0: "explorer/ExplorerPage.qml",
        1: "info/InfoPage.qml",
        2: "library/LibraryPage.qml",
        3: "player/MpvPage.qml",
        4: "download/DownloadPage.qml",
        5: "settings.qml"
    }

    function gotoPage(index){
        if (fullscreen || currentIndex === index) return;
        currentIndex = index
        switch(index) {
        case 1:
            if (!app.currentShow.exists) return;
            break;
        case 3:
            mpv.peak(2000)
            mpvPage.forceActiveFocus()
            mpvPage.visible = true
            stackView.visible = false
            break;
        case 4:
            // @disable-check M126
            if (app.downloader==undefined) return;
            break;
        }

        if (index !== 3) {
            stackView.visible = true
            mpvPage.visible = false
            stackView.replace(pages[index])
        }
    }

    ColumnLayout {
        height: sideBar.height
        spacing: 5
        ImageButton {
            source: selected ? "qrc:/resources/images/search_selected.png" :"qrc:/resources/images/search.png"
            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width
            onClicked: {
                gotoPage(0)
            }
            selected: currentIndex === 0
        }

        ImageButton {
            id: detailsPageButton
            enabled: app.currentShow.exists
            source: selected ? "qrc:/resources/images/details_selected.png" : "qrc:/resources/images/details.png"
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width
            onClicked: gotoPage(1)
            selected: currentIndex == 1
        }

        ImageButton {
            id:libraryPageButton
            source: selected ? "qrc:/resources/images/library_selected.png" :"qrc:/resources/images/library.png"
            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width

            onClicked: gotoPage(2)
            selected: currentIndex === 2
        }

        ImageButton {
            id:playerPageButton
            source: selected ? "qrc:/resources/images/tv_selected.png" :"qrc:/resources/images/tv.png"
            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width
            onClicked: gotoPage(3)
            selected: currentIndex === 3
        }

        ImageButton {
            id: downloadPageButton
            source: selected ? "qrc:/resources/images/download_selected.png" :"qrc:/resources/images/download.png"

            Layout.preferredWidth: sideBar.width
            Layout.preferredHeight: sideBar.width
            onClicked: gotoPage(4)
            selected: currentIndex === 4
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
