import QtQuick
import QtQuick.Controls
import Materialized as M3

Button {
    id: control
    property bool busy: false
    property color containerColor: enabled ? M3.M3Theme.color("primary") : Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, 0.12)
    property color contentColor: enabled ? M3.M3Theme.color("onPrimary") : Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, 0.38)
    property color stateColor: contentColor
    property color tintColor: stateColor
    property int shape: M3.M3Theme.radius("full")
    property real horizontalInset: 24

    implicitWidth: Math.max(64, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: 40
    leftPadding: icon.source ? horizontalInset - 8 : horizontalInset
    rightPadding: horizontalInset
    topPadding: 10
    bottomPadding: 10
    spacing: 8
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    font: M3.M3Theme.font("labelLarge")
    icon.color: contentColor
    icon.width: 18
    icon.height: 18

    contentItem: Item {
        implicitWidth: labelRow.implicitWidth
        implicitHeight: 24
        Row {
            id: labelRow
            anchors.centerIn: parent
            spacing: control.spacing
            Image {
                visible: control.icon.name || control.icon.source
                source: control.icon.source
                sourceSize: Qt.size(18, 18)
                width: 18; height: 18
                anchors.verticalCenter: parent.verticalCenter
            }
            M3Label {
                scale: "labelLarge"
                text: control.text
                textColor: control.contentColor
                visible: !!text
                opacity: control.busy ? 0 : 1
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        M3CircularProgress {
            visible: control.busy
            running: visible
            anchors.centerIn: parent
            width: 20; height: 20
            indicatorColor: control.contentColor
        }
    }

    background: Rectangle {
        implicitWidth: 64
        implicitHeight: 40
        radius: control.down ? M3.M3Theme.radius("small") : control.shape
        color: control.containerColor
        border.width: 0
        // Expressive shape morph: under-damped spatial spring, not a cubic.
        Behavior on radius { SpringAnimation { spring: M3.M3Theme.spring("spatialFast").spring; damping: M3.M3Theme.spring("spatialFast").damping; epsilon: 0.25 } }
        M3StateLayer {
            control: control
            tint: control.stateColor
            radius: parent.radius
        }
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: control.visualFocus ? 3 : 0
            border.color: M3.M3Theme.color("primary")
        }
    }
}
