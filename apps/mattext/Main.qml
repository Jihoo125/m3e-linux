import QtQuick
import QtQuick.Controls
import Materialized as M3

ApplicationWindow {
    id: root
    width: 1080; height: 760
    visible: true
    title: textDocument.modified ? textDocument.title + " •" : textDocument.title
    color: M3.M3Theme.color("background")
    Column {
        anchors.fill: parent
        M3.M3TopAppBar {
            width: parent.width
            headline: textDocument.title
            subtitle: textDocument.status
            M3.M3Button { text: qsTr("Open"); onClicked: fileDialog.open() }
            M3.M3Button { text: qsTr("Save"); enabled: textDocument.modified || textDocument.filePath.length > 0; onClicked: textDocument.save() }
        }
        Rectangle {
            width: parent.width; height: 1; color: M3.M3Theme.color("outlineVariant")
        }
        ScrollView {
            width: parent.width
            height: parent.height - 65
            TextArea {
                id: editor
                width: parent.width
                height: parent.height
                text: textDocument.text
                onTextChanged: if (textDocument.text !== text) textDocument.text = text
                selectByMouse: true
                wrapMode: TextEdit.NoWrap
                font.family: "monospace"
                font.pixelSize: 16
                color: M3.M3Theme.color("onSurface")
                selectionColor: M3.M3Theme.color("primaryContainer")
                selectedTextColor: M3.M3Theme.color("onPrimaryContainer")
                background: Rectangle { color: M3.M3Theme.color("surfaceContainerLowest") }
                leftPadding: 20; topPadding: 20
                persistentSelection: true
            }
        }
    }
    Dialog {
        id: fileDialog
        title: qsTr("Open a text file")
        modal: true
        standardButtons: Dialog.Open | Dialog.Cancel
        width: Math.min(root.width - 64, 600)
        Column {
            width: parent.width
            M3.M3TextField { id: pathField; width: parent.width; label: qsTr("File path"); placeholderText: "/etc/hostname" }
        }
        onAccepted: textDocument.open(pathField.text)
    }
}
