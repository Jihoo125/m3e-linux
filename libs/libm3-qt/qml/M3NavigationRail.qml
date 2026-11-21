import QtQuick
import Materialized as M3

Pane {
    id: control
    property alias model: view.model
    property int currentIndex: -1
    property alias fab: fabSlot.sourceComponent
    default property alias content: view.data
    signal activated(int index)
    implicitWidth: 80
    padding: 0
    background: Rectangle { color: M3.M3Theme.color("surface") }
    contentItem: Item {
        Loader { id: fabSlot; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top; anchors.topMargin: 12 }
        Column {
            id: view
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: fabSlot.item ? fabSlot.bottom : parent.top
            anchors.topMargin: fabSlot.item ? 40 : 44
            spacing: 12
        }
    }
}
