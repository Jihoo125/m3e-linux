import QtQuick
import Materialized as M3

Item {
    id: root
    required property string role
    width: 160; height: 72
    Rectangle {
        anchors.fill: parent; radius: 8
        color: M3.M3Theme.color(root.role)
        border.width: 1; border.color: M3.M3Theme.color("outlineVariant")
    }
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 24; radius: 8
        color: M3.M3Theme.color("surfaceContainer")
    }
    M3.M3Label {
        anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 6
        text: root.role; scale: "labelSmall"
        textColor: M3.M3Theme.color("onSurface")
        width: parent.width - 12; elide: Text.ElideRight
    }
}
