import QtQuick
import QtQuick.Controls
import Materialized as M3

Button {
    id: control
    property bool selected: checked
    property bool first: true
    property bool last: false
    property int total: 1
    checkable: true
    checked: selected
    implicitHeight: 40
    implicitWidth: Math.max(48, contentItem.implicitWidth + 32)
    hoverEnabled: true
    onClicked: selected = checked
    background: Rectangle {
        radius: 0
        color: control.selected ? M3.M3Theme.color("secondaryContainer") : "transparent"
        border.width: 1; border.color: M3.M3Theme.color("outline")
        Rectangle { anchors.fill: parent; color: Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, control.pressed ? 0.10 : control.hovered ? 0.08 : 0) }
    }
    contentItem: Row {
        spacing: 8; anchors.centerIn: parent
        M3Label { text: "✓"; visible: control.selected; textColor: M3.M3Theme.color("onSecondaryContainer"); scale: "labelLarge"; anchors.verticalCenter: parent.verticalCenter }
        M3Label { text: control.text; scale: "labelLarge"; textColor: control.selected ? M3.M3Theme.color("onSecondaryContainer") : M3.M3Theme.color("onSurface"); anchors.verticalCenter: parent.verticalCenter }
    }
}
