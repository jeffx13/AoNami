import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"
import QtQuick.Layouts 1.15


RowLayout {
    id:searchBar
    property alias textField: searchTextField
    property alias providersBox: providersComboBox
    function search(){
        app.search(searchTextField.text, 1)
        root.lastSearch = searchTextField.text
        // parent.forceActiveFocus()
    }

    CustomTextField {
        focusPolicy: Qt.NoFocus
        checkedColor: "#727CF5"
        id:searchTextField
        color: "white"
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.preferredWidth: 5
        placeholderText: qsTr("Enter query!")
        placeholderTextColor: "gray"
        text: root.lastSearch
        font.pixelSize: 20 * root.fontSizeMultiplier
        activeFocusOnTab:false
        onAccepted: search()
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
        onClicked: search()
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
        onClicked: app.latest(1)
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
        onClicked: app.popular(1)
    }

    CustomComboBox {
        id:providersComboBox
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: 2
        contentRadius: 20
        fontSize: 20
        model: app.providerManager
        currentIndex: app.providerManager.currentProviderIndex
        activeFocusOnTab:false
        onActivated: (index) => {app.providerManager.currentProviderIndex = index}
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
        model: app.providerManager.availableShowTypes
        currentIndex: app.providerManager.currentSearchTypeIndex
        currentIndexColor: "red"
        onActivated: (index) => { app.providerManager.currentSearchTypeIndex = index}
    }




    
}



