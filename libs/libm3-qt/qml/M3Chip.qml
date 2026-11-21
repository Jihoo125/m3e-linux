import QtQuick
import QtQuick.Controls
import Materialized as M3

Button {
    id: control
    property string kind: "assist" // assist | filter | input | suggestion
    property bool elevated: false
    property bool selected: checked
    implicitHeight: 32
    leftPadding: control.checkable ? 16 : 16
    rightPadding: 16
    padding: 0
    hoverEnabled: true
    checkable: kind === "filter"
    background: Rectangle {
        implicitHeight: 32
        radius: M3.M3Theme.radius("small")
        color: control.selected ? M3.M3Theme.color("secondaryContainer") : M3.M3Theme.color("surface")
        border.width: control.selected && !control.elevated ? 0 : 1
        border.color: M3.M3Theme.color("outline")
        Rectangle { anchors.fill: parent; radius: parent.radius; color: Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, control.pressed ? 0.10 : control.hovered ? 0.08 : 0) }
    }
    contentItem: Row {
        spacing: 8
        M3Label { text: "✓"; visible: control.checkable && control.checked; scale: "labelLarge"; textColor: M3.M3Theme.color("onSecondaryContainer"); anchors.verticalCenter: parent.verticalCenter }
        Image { source: control.icon.source; sourceSize: Qt.size(18, 18); width: 18; height: 18; visible: !!source; anchors.verticalCenter: parent.verticalCenter }
        M3Label { text: control.text; scale: "labelLarge"; textColor: control.selected ? M3.M3Theme.color("onSecondaryContainer") : M3.M3Theme.color("onSurface"); anchors.verticalCenter: parent.verticalCenter }
    }
}
