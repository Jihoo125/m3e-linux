import QtQuick
import QtQuick.Controls
import Materialized as M3

ApplicationWindow {
    id: root
    width: 1000; height: 720
    visible: true
    title: qsTr("Images")
    color: M3.M3Theme.color("surfaceContainerLowest")

    Column {
        anchors.fill: parent
        M3.M3TopAppBar {
            width: parent.width
            headline: qsTr("Images")
            subtitle: images.info
            M3.M3IconButton { text: "‹"; Accessible.name: qsTr("Previous image"); onClicked: images.previous() }
            M3.M3IconButton { text: "›"; Accessible.name: qsTr("Next image"); onClicked: images.next() }
            M3.M3Button { text: qsTr("Open…"); onClicked: openDialog.open() }
        }
        Item {
            width: parent.width
            height: parent.height - 65
            Image {
                id: picture
                anchors.fill: parent
                anchors.margins: 16
                source: images.currentPath.length ? "file://" + images.currentPath : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                smooth: true
            }
            M3.M3Label {
                anchors.centerIn: parent
                visible: picture.status !== Image.Ready
                text: images.count ? qsTr("Loading…") : qsTr("No images in this folder")
                scale: "titleLarge"
                textColor: M3.M3Theme.color("onSurfaceVariant")
            }
            M3.M3Label {
                anchors.right: parent.right; anchors.bottom: parent.bottom
                anchors.margins: 24
                visible: images.count > 0
                text: qsTr("%1 / %2").arg(images.index + 1).arg(images.count)
                scale: "labelLarge"
                textColor: M3.M3Theme.color("onSurfaceVariant")
            }
            MouseArea {
                anchors.fill: parent
                onWheel: function(wheel) { if (wheel.angleDelta.y > 0) images.previous(); else images.next() }
            }
        }
    }
    Dialog {
        id: openDialog
        title: qsTr("Open image or folder")
        modal: true
        standardButtons: Dialog.Open | Dialog.Cancel
        width: Math.min(root.width - 64, 560)
        Column {
            width: parent.width
            M3.M3TextField { id: pathField; width: parent.width; label: qsTr("Path"); placeholderText: "/usr/share/backgrounds/materialized" }
        }
        onAccepted: {
            if (!images.openFile(pathField.text)) images.openFolder(pathField.text)
        }
    }
    Shortcut { sequence: StandardKey.MoveToNextChar; onActivated: images.next() }
}
