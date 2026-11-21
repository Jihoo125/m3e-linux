import QtQuick
import QtQuick.Controls
import Materialized as M3

MenuItem {
    id: control
    implicitWidth: Math.max(112, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: 48
    hoverEnabled: true
    font: M3.M3Theme.font("bodyLarge")
    icon.width: 24; icon.height: 24
    icon.color: M3.M3Theme.color("onSurfaceVariant")
    arrow: Canvas { width: 24; height: 24; visible: control.subMenu; x: control.width - width - control.rightPadding; anchors.verticalCenter: parent.verticalCenter; onPaint: { var ctx = getContext("2d"); ctx.strokeStyle = M3.M3Theme.color("onSurfaceVariant"); ctx.lineWidth = 2; ctx.beginPath(); ctx.moveTo(8, 6); ctx.lineTo(14, 12); ctx.lineTo(8, 18); ctx.stroke(); } }
    indicator: Item { width: 24; height: 24; visible: control.checkable; x: control.leftPadding; anchors.verticalCenter: parent.verticalCenter; M3Label { anchors.centerIn: parent; text: "✓"; visible: control.checked; textColor: M3.M3Theme.color("onSurface"); scale: "titleMedium" } }
    contentItem: Item {
        implicitWidth: label.implicitWidth + (control.icon.source ? 32 : 0) + (control.checkable ? 24 : 0)
        implicitHeight: 24
        Row {
            anchors.fill: parent
            spacing: 12
            leftPadding: control.checkable ? 28 : 0
            Image { source: control.icon.source; sourceSize: Qt.size(24, 24); width: 24; height: 24; visible: !!source; anchors.verticalCenter: parent.verticalCenter }
            M3Label { id: label; text: control.text; textColor: M3.M3Theme.color("onSurface"); anchors.verticalCenter: parent.verticalCenter }
        }
    }
    background: Rectangle { implicitHeight: 48; color: Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, control.pressed ? 0.12 : control.highlighted ? 0.08 : control.hovered ? 0.08 : 0) }
}
