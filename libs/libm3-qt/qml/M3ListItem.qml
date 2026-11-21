import QtQuick
import QtQuick.Controls
import Materialized as M3

ItemDelegate {
    id: control
    property string supportingText: ""
    property string overline: ""
    property string leadingIcon: ""
    property string trailingText: ""
    property string trailingIcon: ""
    property bool selected: false
    implicitHeight: overline ? 88 : supportingText ? 72 : 56
    implicitWidth: 200
    padding: 16
    hoverEnabled: true
    highlighted: selected
    background: Rectangle {
        color: Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, control.pressed ? 0.10 : control.hovered ? 0.08 : control.selected ? 0.08 : 0)
    }
    contentItem: Row {
        spacing: 16
        Image { source: control.leadingIcon; sourceSize: Qt.size(24, 24); width: 24; height: 24; visible: !!source; anchors.verticalCenter: parent.verticalCenter }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - (leadingImage.visible ? 40 : 0) - trailing.width - 16
            M3Label { text: control.overline; scale: "labelSmall"; textColor: M3.M3Theme.color("onSurfaceVariant"); visible: !!text }
            M3Label { text: control.text; scale: "bodyLarge"; textColor: M3.M3Theme.color("onSurface") }
            M3Label { text: control.supportingText; scale: "bodyMedium"; textColor: M3.M3Theme.color("onSurfaceVariant"); visible: !!text }
        }
        Item { id: leadingImage; visible: false; width: 0; height: 0 }
        Row {
            id: trailing
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            M3Label { text: control.trailingText; scale: "labelSmall"; textColor: M3.M3Theme.color("onSurfaceVariant"); visible: !!text; anchors.verticalCenter: parent.verticalCenter }
            Image { source: control.trailingIcon; sourceSize: Qt.size(24, 24); width: 24; height: 24; visible: !!source }
        }
    }
}
