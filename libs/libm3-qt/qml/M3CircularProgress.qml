import QtQuick
import Materialized as M3

Item {
    id: control
    property real value: 0 // 0..1
    property bool running: false
    property color indicatorColor: M3.M3Theme.color("primary")
    property color trackColor: M3.M3Theme.color("surfaceVariant")
    property int strokeWidth: 4
    implicitWidth: 48; implicitHeight: 48
    Canvas {
        id: canvas
        anchors.fill: parent
        rotation: control.running ? 0 : -90
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const w = width; const h = height
            const cx = w / 2; const cy = h / 2
            const r = Math.min(w, h) / 2 - control.strokeWidth / 2
            ctx.lineWidth = control.strokeWidth
            ctx.lineCap = "round"
            ctx.strokeStyle = control.trackColor
            ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI * 2); ctx.stroke()
            ctx.strokeStyle = control.indicatorColor
            const start = control.running ? (rotationAngle % 360) * Math.PI / 180 : 0
            const extent = control.running ? Math.PI / 2 : Math.max(0, Math.min(control.value, 1)) * Math.PI * 2
            ctx.beginPath(); ctx.arc(cx, cy, r, start, start + extent); ctx.stroke()
        }
        property real rotationAngle: 0
        NumberAnimation on rotationAngle { target: canvas; from: 0; to: 360; duration: 1200; loops: Animation.Infinite; running: control.running && control.visible }
        onRotationAngleChanged: if (control.running) requestPaint()
        Connections { target: control; function onValueChanged() { canvas.requestPaint() } function onRunningChanged() { canvas.requestPaint() } }
        Component.onCompleted: requestPaint()
    }
}
