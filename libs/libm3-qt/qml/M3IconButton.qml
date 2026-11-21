import QtQuick
import QtQuick.Controls
import Materialized as M3

ToolButton {
    id: control
    property bool selected: false
    property bool toggleable: false
    property color contentColor: selected ? M3.M3Theme.color("onPrimary") : M3.M3Theme.color("onSurfaceVariant")
    property color containerColor: selected ? M3.M3Theme.color("primary") : "transparent"
    implicitWidth: 40
    implicitHeight: 40
    hoverEnabled: true
    checkable: toggleable
    padding: 8
    icon.width: 24; icon.height: 24
    icon.color: contentColor
    onClicked: if (toggleable) selected = checked

    contentItem: Item {
        implicitWidth: 24; implicitHeight: 24
        Image {
            anchors.centerIn: parent
            source: control.icon.source
            sourceSize: Qt.size(24, 24)
            width: 24; height: 24
            opacity: control.enabled ? 1 : 0.38
        }
        M3Label {
            anchors.centerIn: parent
            visible: !control.icon.source
            text: control.text
            textColor: control.contentColor
            scale: "titleMedium"
        }
    }
    background: Rectangle {
        width: control.width; height: control.height
        radius: width / 2
        color: control.containerColor
        M3StateLayer { control: control; tint: control.contentColor }
        Rectangle { anchors.fill: parent; radius: parent.radius; color: "transparent"; border.width: control.visualFocus ? 2 : 0; border.color: M3.M3Theme.color("primary") }
    }
}
