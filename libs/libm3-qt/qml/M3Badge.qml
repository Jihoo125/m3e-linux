import QtQuick
import QtQuick.Controls
import Materialized as M3

Item {
    id: control
    property string text: ""
    property color badgeColor: M3.M3Theme.color("error")
    property color contentColor: M3.M3Theme.color("onError")
    property bool dot: !text
    implicitWidth: dot ? 6 : Math.max(16, label.implicitWidth + 10)
    implicitHeight: dot ? 6 : 16
    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: control.badgeColor
        M3Label { id: label; anchors.centerIn: parent; text: control.text; scale: "labelSmall"; textColor: control.contentColor; font.pixelSize: 11 }
    }
}
