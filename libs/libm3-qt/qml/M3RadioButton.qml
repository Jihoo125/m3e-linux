import QtQuick
import QtQuick.Controls
import Materialized as M3

RadioButton {
    id: control
    implicitWidth: 20
    implicitHeight: 20
    hoverEnabled: true
    padding: 0
    indicator: Rectangle {
        width: 20; height: 20; radius: 10
        color: "transparent"
        border.width: 2
        border.color: control.checked ? M3.M3Theme.color("primary") : M3.M3Theme.color("onSurfaceVariant")
        opacity: control.enabled ? 1 : 0.38
        Rectangle {
            anchors.centerIn: parent; width: 10; height: 10; radius: 5
            color: M3.M3Theme.color("primary")
            scale: control.checked ? 1 : 0
            Behavior on scale { NumberAnimation { duration: M3.M3Theme.duration("short3"); easing.type: Easing.OutCubic } }
        }
        Rectangle { anchors.centerIn: parent; width: 40; height: 40; radius: 20; color: Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, control.pressed ? 0.10 : control.hovered ? 0.08 : 0) }
    }
    contentItem: M3Label {
        visible: !!control.text
        text: control.text
        textColor: M3.M3Theme.color("onSurface")
        leftPadding: control.indicator.width + 12
        verticalAlignment: Text.AlignVCenter
    }
}
