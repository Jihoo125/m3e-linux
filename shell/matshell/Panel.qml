import QtQuick
import Materialized as M3

Rectangle {
    id: root
    height: 56
    color: M3.M3Theme.color("surfaceContainer")
    signal launcherRequested()
    signal quickSettingsRequested()
    signal notificationsRequested()

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: clock.time = new Date()
    }
    Row {
        anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        M3.M3IconButton {
            text: "◈"
            selected: true
            Accessible.name: qsTr("Open application launcher")
            onClicked: root.launcherRequested()
        }
        M3.M3Label { text: qsTr("Workspace 1"); scale: "labelLarge"; anchors.verticalCenter: parent.verticalCenter }
        Repeater { model: 4
            Rectangle {
                width: 6; height: 6; radius: 3
                color: index === 0 ? M3.M3Theme.color("primary") : M3.M3Theme.color("outlineVariant")
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
    M3.M3Label {
        id: clock
        property date time: new Date()
        anchors.centerIn: parent
        scale: "labelLarge"
        text: Qt.formatDateTime(clock.time, "ddd MMM d  hh:mm")
    }
    Row {
        anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        M3.M3Label { text: qsTr("Wi-Fi"); scale: "labelMedium"; anchors.verticalCenter: parent.verticalCenter }
        M3.M3IconButton {
            text: notificationServer ? (notificationServer.doNotDisturb ? "◌" : "●") : "●"
            Accessible.name: qsTr("Notification center")
            onClicked: root.notificationsRequested()
        }
        M3.M3IconButton {
            text: "⋮"
            Accessible.name: qsTr("Quick settings")
            onClicked: root.quickSettingsRequested()
        }
    }
}
