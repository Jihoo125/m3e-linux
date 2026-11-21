import QtQuick
import QtQuick.Controls
import Materialized as M3

CheckBox {
    id: control
    implicitWidth: 18
    implicitHeight: 18
    hoverEnabled: true
    padding: 0
    indicator: Rectangle {
        width: 18; height: 18
        radius: 2
        color: control.checked || control.checkState === Qt.PartiallyChecked ? M3.M3Theme.color("primary") : "transparent"
        border.width: control.checked || control.checkState === Qt.PartiallyChecked ? 0 : 2
        border.color: M3.M3Theme.color("onSurfaceVariant")
        opacity: control.enabled ? 1 : 0.38
        Rectangle {
            anchors.centerIn: parent; width: 40; height: 40; radius: 20
            color: Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, control.pressed ? 0.10 : control.hovered ? 0.08 : 0)
        }
        Text {
            anchors.centerIn: parent
            text: control.checkState === Qt.PartiallyChecked ? "−" : "✓"
            color: M3.M3Theme.color("onPrimary")
            font: M3.M3Theme.font("labelLarge")
            visible: control.checked || control.checkState === Qt.PartiallyChecked
        }
        Rectangle { anchors.fill: parent; radius: 2; color: "transparent"; border.width: control.visualFocus ? 2 : 0; border.color: M3.M3Theme.color("primary") }
    }
    contentItem: M3Label {
        visible: !!control.text
        text: control.text
        textColor: M3.M3Theme.color("onSurface")
        leftPadding: control.indicator.width + 12
        verticalAlignment: Text.AlignVCenter
    }
}
