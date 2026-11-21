import QtQuick
import Materialized as M3

Rectangle {
    id: root
    width: 400
    color: M3.M3Theme.color("surfaceContainer")
    signal dismissed()

    Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16
        M3.M3Label { text: qsTr("Notifications"); scale: "titleLarge" }
        M3.M3Label { text: notificationServer && notificationServer.model.length === 0 ? qsTr("You're all caught up") : ""; scale: "bodyLarge" }
        ListView {
            width: parent.width
            height: parent.height - 56
            model: notificationServer ? notificationServer.model : []
            spacing: 12
            delegate: M3.M3Card {
                width: parent.width
                Column {
                    anchors.left: parent.left; anchors.right: parent.right
                    spacing: 8
                    M3.M3Label { text: modelData.application; scale: "labelSmall"; textColor: M3.M3Theme.color("onSurfaceVariant") }
                    M3.M3Label { text: modelData.summary; scale: "titleMedium" }
                    M3.M3Label { text: modelData.body; scale: "bodyMedium"; wrapMode: Text.WordWrap; width: parent.width }
                    M3.M3TextButton { text: qsTr("Dismiss"); onClicked: notificationServer.closeNotification(modelData.id) }
                }
            }
        }
    }
}
