import QtQuick
import Materialized as M3

M3Button {
    id: control
    containerColor: "transparent"
    contentColor: enabled ? M3.M3Theme.color("primary") : Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, 0.38)
    background: Rectangle {
        radius: control.down ? M3.M3Theme.radius("small") : control.shape
        color: control.containerColor
        border.width: 1
        border.color: control.enabled ? M3.M3Theme.color("outline") : Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, 0.12)
        M3StateLayer { control: control; tint: control.stateColor; radius: parent.radius }
        Rectangle { anchors.fill: parent; radius: parent.radius; color: "transparent"; border.width: control.visualFocus ? 3 : 0; border.color: M3.M3Theme.color("primary") }
    }
}
