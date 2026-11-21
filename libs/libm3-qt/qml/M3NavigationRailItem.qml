import QtQuick
import QtQuick.Controls
import Materialized as M3

ItemDelegate {
    id: control
    property string icon: ""
    property string selectedIcon: icon
    property string badge: ""
    property bool active: checked || highlighted
    implicitWidth: 56
    implicitHeight: 56
    padding: 0
    hoverEnabled: true
    checkable: true
    background: Item { implicitWidth: 56; implicitHeight: 56 }
    contentItem: Column {
        spacing: 4
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 56; height: 32; radius: 16
            color: control.active ? M3.M3Theme.color("secondaryContainer") : Qt.rgba(M3.M3Theme.color("onSurfaceVariant").r, M3.M3Theme.color("onSurfaceVariant").g, M3.M3Theme.color("onSurfaceVariant").b, control.pressed ? 0.12 : control.hovered ? 0.08 : 0)
            Image { anchors.centerIn: parent; source: control.active ? control.selectedIcon : control.icon; sourceSize: Qt.size(24, 24); width: 24; height: 24 }
            M3Badge { anchors.right: parent.right; anchors.top: parent.top; text: control.badge; visible: !!text }
        }
        M3Label { text: control.text; visible: !!text; scale: "labelMedium"; textColor: control.active ? M3.M3Theme.color("onSurface") : M3.M3Theme.color("onSurfaceVariant"); anchors.horizontalCenter: parent.horizontalCenter }
    }
}
