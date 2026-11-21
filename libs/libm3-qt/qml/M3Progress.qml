import QtQuick
import QtQuick.Controls
import Materialized as M3

ProgressBar {
    id: control
    property bool running: false
    implicitWidth: 200
    implicitHeight: 4
    padding: 0
    indeterminate: running
    background: Rectangle { implicitHeight: 4; radius: 2; color: M3.M3Theme.color("surfaceVariant") }
    contentItem: Item {
        implicitHeight: 4
        Rectangle {
            width: control.indeterminate ? parent.width * 0.35 : control.visualPosition * parent.width
            height: 4; radius: 2
            color: M3.M3Theme.color("primary")
            anchors.left: parent.left
            anchors.leftMargin: control.indeterminate ? indeterminateX : 0
            property real indeterminateX: 0
            SequentialAnimation on indeterminateX {
                running: control.indeterminate && control.visible
                loops: Animation.Infinite
                NumberAnimation { from: 0; to: control.width * 0.65; duration: 900; easing.type: Easing.InOutCubic }
                NumberAnimation { to: 0; duration: 0 }
            }
        }
        Rectangle { width: 4; height: 4; radius: 2; color: M3.M3Theme.color("primary"); anchors.right: parent.right; visible: !control.indeterminate }
    }
}
