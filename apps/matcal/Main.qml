import QtQuick
import QtQuick.Controls
import Materialized as M3

ApplicationWindow {
    id: root
    width: 1000; height: 700
    visible: true
    title: qsTr("Calendar")
    color: M3.M3Theme.color("background")

    Row {
        anchors.fill: parent
        Column {
            width: parent.width * 0.62
            height: parent.height
            padding: 24
            spacing: 16
            Row {
                spacing: 12
                M3.M3IconButton { text: "‹"; Accessible.name: qsTr("Previous month"); onClicked: calendar.previousMonth() }
                M3.M3Label {
                    text: Qt.formatDate(calendar.month, "MMMM yyyy")
                    scale: "headlineSmall"; emphasized: true
                    anchors.verticalCenter: parent.verticalCenter
                }
                M3.M3IconButton { text: "›"; Accessible.name: qsTr("Next month"); onClicked: calendar.nextMonth() }
            }
            Grid {
                columns: 7
                spacing: 4
                Repeater {
                    model: [qsTr("Mo"), qsTr("Tu"), qsTr("We"), qsTr("Th"), qsTr("Fr"), qsTr("Sa"), qsTr("Su")]
                    delegate: M3.M3Label { width: 72; horizontalAlignment: Text.AlignHCenter; text: modelData; scale: "labelMedium"; textColor: M3.M3Theme.color("onSurfaceVariant") }
                }
                Repeater {
                    model: calendar.monthGrid
                    delegate: Rectangle {
                        width: 72; height: 64
                        radius: M3.M3Theme.radius("medium")
                        color: modelData.selected ? M3.M3Theme.color("primaryContainer") : modelData.today ? M3.M3Theme.color("secondaryContainer") : "transparent"
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            M3.M3Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData.day
                                scale: "titleMedium"
                                textColor: modelData.inMonth ? M3.M3Theme.color("onSurface") : M3.M3Theme.color("outlineVariant")
                            }
                            Rectangle {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: 6; height: 6; radius: 3
                                visible: modelData.hasEvents
                                color: M3.M3Theme.color("primary")
                            }
                        }
                        MouseArea { anchors.fill: parent; onClicked: calendar.selected = new Date(modelData.date) }
                    }
                }
            }
        }
        Rectangle {
            width: parent.width * 0.38
            height: parent.height
            color: M3.M3Theme.color("surfaceContainerLow")
            Column {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 12
                M3.M3Label { text: Qt.formatDate(calendar.selected, "dddd, d MMMM"); scale: "titleLarge"; emphasized: true }
                M3.M3TextField { id: titleField; width: parent.width; label: qsTr("New event") }
                M3.M3TextField { id: timeField; width: parent.width; label: qsTr("Time (optional)"); placeholderText: "14:30" }
                M3.M3Button {
                    text: qsTr("Add event")
                    onClicked: { calendar.addEvent(titleField.text, timeField.text); titleField.text = ""; timeField.text = "" }
                }
                M3.M3Divider { width: parent.width }
                ListView {
                    width: parent.width
                    height: parent.height - 220
                    model: calendar
                    clip: true
                    delegate: Rectangle {
                        width: parent ? parent.width : 0
                        height: 56
                        radius: M3.M3Theme.radius("small")
                        color: index % 2 ? "transparent" : M3.M3Theme.color("surfaceContainer")
                        Row {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 12
                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                M3.M3Label { text: title; scale: "bodyLarge" }
                                M3.M3Label { text: time; scale: "bodySmall"; textColor: M3.M3Theme.color("onSurfaceVariant"); visible: time.length > 0 }
                            }
                        }
                        M3.M3TextButton { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: qsTr("Delete"); onClicked: calendar.removeEvent(eventId) }
                    }
                }
            }
        }
    }
}
