import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"
import QtQuick.Layouts 1.15
import Kyokou 1.0

RowLayout {
    id:searchBar
    property alias textField: searchTextField
    property alias providersBox: providersComboBox
    function search(){
        App.explore(searchTextField.text, 1, false)
    }
    Component.onDestruction: {
        root.lastSearch = searchTextField.text
    }

    CustomTextField {
        focusPolicy: Qt.NoFocus
        checkedColor: "#727CF5"
        id: searchTextField
        color: "white"
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.preferredWidth: 5
        placeholderText: qsTr("Enter query!")
        placeholderTextColor: "gray"
        text: root.lastSearch
        font.pixelSize: 20 * root.fontSizeMultiplier
        activeFocusOnTab:false
        onAccepted: searchBar.search()
    }
    
    CustomButton {
        id: searchButton
        text: "Search"
        Layout.fillHeight: true
        Layout.preferredWidth: 1
        Layout.fillWidth: true
        fontSize:20
        radius: 20
        activeFocusOnTab:false
        focusPolicy: Qt.NoFocus
        onClicked: searchBar.search()
    }

    CustomButton {
        id: latestButton
        text: "Latest"
        Layout.fillHeight: true
        Layout.fillWidth: true
        radius: 20
        // fontSize:20
        activeFocusOnTab:false
        Layout.preferredWidth: 1
        focusPolicy: Qt.NoFocus
        onClicked: App.explore("", 1, true)
    }
    
    CustomButton {
        id: popularButton
        text: "Popular"
        Layout.fillHeight: true
        Layout.fillWidth: true
        fontSize:20
        radius: 20
        activeFocusOnTab:false
        Layout.preferredWidth: 1
        focusPolicy: Qt.NoFocus
        onClicked: App.explore("", 1, false)
    }

    CustomComboBox {
        id:providersComboBox
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: 2
        contentRadius: 20
        fontSize: 20
        model: App.providerManager
        currentIndex: App.providerManager.currentProviderIndex
        activeFocusOnTab: false
        onActivated: (index) => {App.providerManager.currentProviderIndex = index}
        text: "text"

    }

    CustomComboBox {
        id:typeComboBox
        Layout.preferredWidth: 2
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentRadius: 20
        fontSize: 20
        activeFocusOnTab:false
        model: App.providerManager.availableShowTypes
        currentIndex: App.providerManager.currentSearchTypeIndex
        currentIndexColor: "red"
        onActivated: (index) => { App.providerManager.currentSearchTypeIndex = index}
    }




    
}



