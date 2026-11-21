import QtQuick
import QtQuick.Controls
import Materialized as M3

ApplicationWindow {
    id: root
    width: 1100; height: 740
    visible: true
    title: qsTr("Files · %1").arg(files.directory)
    color: M3.M3Theme.color("background")
    Column {
        anchors.fill: parent
        M3.M3TopAppBar {
            width: parent.width
            headline: qsTr("Files")
            subtitle: files.directory
            M3.M3IconButton { text: "↑"; Accessible.name: qsTr("Parent folder"); onClicked: files.up() }
            M3.M3IconButton { text: "⌂"; Accessible.name: qsTr("Home"); onClicked: files.home() }
            M3.M3IconButton { text: "+"; Accessible.name: qsTr("New folder"); onClicked: newFolder.open() }
        }
        M3.M3TextField { width: parent.width; label: qsTr("Search this folder"); onTextChanged: files.filter = text }
        ListView {
            width: parent.width; height: root.height - 140
            model: files
            clip: true
            delegate: Rectangle {
                width: parent.width; height: 58
                color: index % 2 ? "transparent" : M3.M3Theme.color("surfaceContainerLow")
                Row {
                    anchors.fill: parent; anchors.margins: 12; spacing: 16
                    M3.M3Label { text: isDirectory ? "▰" : "▱"; scale: "titleMedium"; width: 24 }
                    M3.M3Label { text: name; width: parent.width - 240; scale: "bodyLarge" }
                    M3.M3Label { text: size; width: 100; scale: "bodySmall"; textColor: M3.M3Theme.color("onSurfaceVariant") }
                    M3.M3TextButton { text: qsTr("Delete"); onClicked: files.remove(index) }
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onDoubleClicked: files.open(index)
                    onClicked: parent.forceActiveFocus()
                }
            }
        }
    }
    Dialog {
        id: newFolder
        title: qsTr("New folder")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        Column { width: parent.width; M3.M3TextField { id: folderName; width: parent.width; label: qsTr("Name") } }
        onAccepted: files.createDirectory(folderName.text)
    }
}
