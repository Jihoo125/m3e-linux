import QtQuick
import QtQuick.Controls
import Materialized as M3

ApplicationWindow {
    id: root
    property alias page: contentArea.children
    property alias appBar: headerLoader.sourceComponent
    property alias navigation: footerLoader.sourceComponent
    property string titleText: title
    default property alias content: contentArea.data
    width: 1280
    height: 800
    minimumWidth: 360
    minimumHeight: 480
    visible: true
    color: M3.M3Theme.color("background")
    flags: Qt.Window | Qt.FramelessWindowHint
    title: "Materialized"

    Rectangle { anchors.fill: parent; color: root.color; radius: 12; layer.enabled: false }
    Column {
        anchors.fill: parent
        Loader { id: headerLoader; width: parent.width; active: !!sourceComponent }
        Item { id: contentArea; width: parent.width; height: parent.height - (headerLoader.item ? headerLoader.height : 0) - (footerLoader.item ? footerLoader.height : 0); clip: true }
        Loader { id: footerLoader; width: parent.width; active: !!sourceComponent }
    }
    // A keyboard-accessible fallback when server-side decorations are unavailable.
    M3IconButton {
        id: closeButton
        anchors.right: parent.right; anchors.top: parent.top
        anchors.rightMargin: 8; anchors.topMargin: 8
        text: "×"
        Accessible.name: qsTr("Close")
        visible: Qt.platform.pluginName !== "wayland"
        onClicked: root.close()
        z: 10
    }
    DragHandler {
        target: null
        onActiveChanged: if (active) root.startSystemMove()
        grabPermissions: PointerHandler.TakeOverForbidden
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        propagateComposedEvents: true
        onPressed: mouse.accepted = false
    }
}
