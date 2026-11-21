import QtQuick
import Materialized as M3

Rectangle {
    id: root
    width: 360
    color: M3.M3Theme.color("surfaceContainer")
    signal dismissed()

    Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        M3.M3Label { text: qsTr("Quick settings"); scale: "titleLarge" }
        M3.M3Label { text: qsTr("Appearance"); scale: "labelLarge"; textColor: M3.M3Theme.color("onSurfaceVariant") }
        M3.M3Switch { text: qsTr("Dark theme"); checked: M3.M3Theme.dark; onClicked: themeInterface.setMode(checked ? "dark" : "light") }
        M3.M3Divider { width: parent.width }
        M3.M3Label { text: qsTr("Volume"); scale: "labelLarge" }
        M3.M3Slider { width: parent.width; from: 0; to: 100; value: devices.volume; showValue: true; onMoved: devices.volume = value }
        M3.M3Switch { text: qsTr("Mute"); checked: devices.muted; onClicked: devices.muted = checked }
        M3.M3Label { text: qsTr("Brightness"); scale: "labelLarge" }
        M3.M3Slider { width: parent.width; from: 0; to: 100; value: devices.brightness; showValue: true; onMoved: devices.brightness = value }
        M3.M3Divider { width: parent.width }
        M3.M3Switch { text: qsTr("Do not disturb"); checked: notificationServer ? notificationServer.doNotDisturb : false; onClicked: notificationServer.setDoNotDisturb(checked) }
        M3.M3Button { text: qsTr("Close"); onClicked: root.dismissed() }
    }
}
