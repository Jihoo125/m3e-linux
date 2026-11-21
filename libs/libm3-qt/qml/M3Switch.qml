import QtQuick
import QtQuick.Controls
import Materialized as M3

Switch {
    id: control
    implicitWidth: 52
    implicitHeight: 32
    padding: 0
    hoverEnabled: true

    property color selectedTrackColor: M3.M3Theme.color("primary")
    property color unselectedTrackColor: M3.M3Theme.color("surfaceVariant")
    property color selectedThumbColor: M3.M3Theme.color("onPrimary")
    property color unselectedThumbColor: M3.M3Theme.color("outline")

    indicator: Item {
        implicitWidth: 52; implicitHeight: 32
        x: control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        Rectangle {
            id: track
            width: 52; height: 32
            radius: 16
            color: control.enabled ? (control.checked ? control.selectedTrackColor : control.unselectedTrackColor) : Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, 0.12)
            border.width: control.checked ? 0 : 2
            border.color: M3.M3Theme.color("outline")
            M3StateLayer { control: control; tint: control.checked ? M3.M3Theme.color("onPrimary") : M3.M3Theme.color("onSurfaceVariant") }
            Rectangle {
                id: thumb
                property real thumbSize: control.pressed ? 28 : (control.checked ? 24 : 16)
                width: thumbSize; height: thumbSize
                radius: width / 2
                x: control.checked ? track.width - width - (control.pressed ? 2 : 4) : (control.pressed ? 2 : 8)
                anchors.verticalCenter: parent.verticalCenter
                color: control.checked ? control.selectedThumbColor : control.unselectedThumbColor
                opacity: control.enabled ? 1 : 0.38
                Behavior on x { SpringAnimation { spring: M3.M3Theme.spring("spatialDefault").spring; damping: M3.M3Theme.spring("spatialDefault").damping; epsilon: 0.25 } }
                Behavior on thumbSize { SpringAnimation { spring: M3.M3Theme.spring("spatialFast").spring; damping: M3.M3Theme.spring("spatialFast").damping; epsilon: 0.25 } }
                Behavior on color { ColorAnimation { duration: M3.M3Theme.duration("short3") } }
                M3.M3Label { text: control.checked ? "✓" : ""; scale: "labelMedium"; textColor: M3.M3Theme.color("primary"); anchors.centerIn: parent; visible: control.checked }
            }
        }
    }
    contentItem: Item {}
}
