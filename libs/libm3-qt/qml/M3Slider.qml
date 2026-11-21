import QtQuick
import QtQuick.Controls
import Materialized as M3

Slider {
    id: control
    implicitWidth: 200
    implicitHeight: 44
    hoverEnabled: true
    stepSize: 0.0
    property real tickInterval: 0
    property bool showValue: false

    background: Item {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: 16
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width; height: 4; radius: 2
            color: M3.M3Theme.color("secondaryContainer")
            opacity: control.enabled ? 1 : 0.38
        }
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: control.visualPosition * parent.width
            height: 4; radius: 2
            color: M3.M3Theme.color("primary")
            opacity: control.enabled ? 1 : 0.38
        }
        Row {
            visible: control.tickInterval > 0
            width: parent.width; height: 4
            Repeater {
                model: control.tickInterval > 0 ? Math.floor((control.to - control.from) / control.tickInterval) + 1 : 0
                delegate: Rectangle {
                    x: index * (parent.width - 4) / Math.max(1, Math.floor((control.to - control.from) / control.tickInterval))
                    width: 4; height: 4; radius: 2
                    color: M3.M3Theme.color("onPrimary")
                }
            }
        }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.pressed ? 4 : 4; height: 44
        radius: 2
        color: M3.M3Theme.color("primary")
        opacity: control.enabled ? 1 : 0.38
        Rectangle {
            anchors.centerIn: parent
            width: 32; height: 32; radius: 16
            color: Qt.rgba(M3.M3Theme.color("primary").r, M3.M3Theme.color("primary").g, M3.M3Theme.color("primary").b, control.pressed ? 0.16 : control.hovered ? 0.08 : 0)
        }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: -36
            width: valueLabel.implicitWidth + 16; height: 26; radius: 13
            color: M3.M3Theme.color("inverseSurface")
            visible: control.pressed || control.visualFocus || control.showValue
            M3Label { id: valueLabel; anchors.centerIn: parent; text: control.value.toFixed(control.stepSize > 0 && control.stepSize < 1 ? 1 : 0); scale: "labelMedium"; textColor: M3.M3Theme.color("inverseOnSurface") }
        }
    }
}
