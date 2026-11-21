import QtQuick
import QtQuick.Controls
import Materialized as M3

/// Animated tint for hover, focus and press states. The alpha is part of the
/// colour binding so ColorAnimation works on Qt 6.2 as well as newer Qt.
Rectangle {
    id: root
    property color tint: M3.M3Theme.color("onSurface")
    required property Item control
    property bool dragging: false
    readonly property real stateOpacity: {
        if (!control || !control.enabled) return 0.0
        if (control.pressed || dragging) return 0.10
        if (control.visualFocus) return 0.12
        if (control.hovered) return 0.08
        return 0.0
    }
    anchors.fill: parent
    radius: parent.radius
    color: Qt.rgba(tint.r, tint.g, tint.b, stateOpacity)
    visible: stateOpacity > 0
    onStateOpacityChanged: visible = stateOpacity > 0
    Behavior on color {
        ColorAnimation {
            duration: M3.M3Theme.duration("short3")
            easing.type: Easing.Bezier
            easing.bezierCurve: M3.M3Theme.easing("standard")
        }
    }
}
