pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import AoNami

AppMenu {
    id: typeMenu

    // -1 when the show is not in the library yet.
    property int currentType: -1
    signal picked(int type)

    title: currentType === -1 ? "Add To Library" : "Change Library Type"

    Instantiator {
        model: App.library.typeNames
        delegate: Action {
            required property int    index
            required property string modelData
            text: modelData
        }
        onObjectAdded: (i, obj) => {
            obj.enabled = Qt.binding(() => typeMenu.currentType !== obj.index)
            obj.triggered.connect(() => typeMenu.picked(obj.index))
            typeMenu.insertAction(i, obj)
        }
        onObjectRemoved: (i, obj) => typeMenu.removeAction(obj)
    }
}
