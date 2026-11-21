import QtQuick
import QtQuick.Controls
import Materialized as M3

Rectangle {
    id: root
    signal dismissed()
    color: Qt.rgba(M3.M3Theme.color("background").r, M3.M3Theme.color("background").g, M3.M3Theme.color("background").b, 0.98)

    Column {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 24
        M3.M3TextField {
            id: search
            width: Math.min(parent.width, 720)
            anchors.horizontalCenter: parent.horizontalCenter
            label: qsTr("Search applications")
            placeholderText: qsTr("Type a name or description")
            onTextChanged: applicationModel.query = text
            Keys.onEscapePressed: root.dismissed()
        }
        M3.M3Label {
            text: applicationModel.count === 0 ? qsTr("No applications match your search") : qsTr("Applications")
            scale: "titleLarge"
        }
        GridView {
            width: parent.width
            height: parent.height - search.height - 80
            clip: true
            cellWidth: 144
            cellHeight: 128
            model: applicationModel
            delegate: M3.M3Card {
                width: 128; height: 112
                interactive: true
                tone: "filled"
                Column {
                    anchors.centerIn: parent
                    spacing: 8
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 40; height: 40; radius: 20
                        color: M3.M3Theme.color("primaryContainer")
                        M3.M3Label { anchors.centerIn: parent; text: name.length > 0 ? name.charAt(0).toUpperCase() : "?"; scale: "titleLarge"; textColor: M3.M3Theme.color("onPrimaryContainer") }
                    }
                    M3.M3Label { text: name; scale: "labelLarge"; width: 112; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                    M3.M3Label { text: comment; scale: "bodySmall"; visible: false }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: { if (applicationModel.launch(index)) root.dismissed() }
                }
            }
        }
    }
    MouseArea { anchors.fill: parent; z: -1; onClicked: root.dismissed() }
}
