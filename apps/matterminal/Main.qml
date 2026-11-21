import QtQuick
import QtQuick.Controls
import Materialized as M3

ApplicationWindow {
    id: root
    width: 900; height: 600
    visible: true
    title: qsTr("Terminal")
    color: M3.M3Theme.color("surfaceContainerLowest")

    FontMetrics { id: metrics; font.family: "monospace"; font.pixelSize: 14 }

    function cols() { return Math.max(20, Math.floor(termArea.width / metrics.advanceWidth("M"))) }
    function rows() { return Math.max(5, Math.floor(termArea.height / metrics.height)) }
    function syncSize() { pty.resize(cols(), rows()) }

    Column {
        anchors.fill: parent
        M3.M3TopAppBar {
            width: parent.width
            headline: qsTr("Terminal")
            subtitle: pty.running ? qsTr("bash · %1×%2").arg(terminal.columns).arg(terminal.rows) : qsTr("session ended")
            M3.M3Button { text: qsTr("Restart"); visible: !pty.running; onClicked: pty.start(root.cols(), root.rows()) }
        }
        Rectangle {
            id: termArea
            width: parent.width
            height: parent.height - 65
            color: M3.M3Theme.color("surfaceContainerLowest")
            ListView {
                id: view
                anchors.fill: parent
                anchors.margins: 8
                model: terminal.lines
                clip: true
                interactive: false
                delegate: Text {
                    width: view.width
                    text: modelData
                    textFormat: Text.StyledText
                    font: metrics.font
                    color: M3.M3Theme.color("onSurface")
                }
                onCountChanged: positionViewAtEnd()
            }
            // Blinking block cursor at the emulator's cursor position.
            Rectangle {
                x: 8 + terminal.cursorX * metrics.advanceWidth("M")
                y: 8 + terminal.cursorY * metrics.height
                width: metrics.advanceWidth("M"); height: metrics.height
                color: M3.M3Theme.color("primary")
                opacity: cursorTimer.on ? 0.6 : 0.15
                Timer { id: cursorTimer; property bool on: true; interval: 530; repeat: true; running: pty.running; onTriggered: on = !on }
            }
            MouseArea { anchors.fill: parent; onClicked: keySink.forceActiveFocus() }
        }
    }

    Item {
        id: keySink
        anchors.fill: parent
        focus: true
        Keys.onPressed: function(event) {
            if (!pty.running) return
            let bytes = ""
            if (event.modifiers & Qt.ControlModifier && event.key >= Qt.Key_A && event.key <= Qt.Key_Z) {
                bytes = String.fromCharCode(event.key - Qt.Key_A + 1)
            } else {
                switch (event.key) {
                case Qt.Key_Return: case Qt.Key_Enter: bytes = "\r"; break
                case Qt.Key_Backspace: bytes = "\x7f"; break
                case Qt.Key_Tab: bytes = "\t"; break
                case Qt.Key_Escape: bytes = "\x1b"; break
                case Qt.Key_Up: bytes = "\x1b[A"; break
                case Qt.Key_Down: bytes = "\x1b[B"; break
                case Qt.Key_Right: bytes = "\x1b[C"; break
                case Qt.Key_Left: bytes = "\x1b[D"; break
                case Qt.Key_Home: bytes = "\x1b[H"; break
                case Qt.Key_End: bytes = "\x1b[F"; break
                case Qt.Key_Delete: bytes = "\x1b[3~"; break
                case Qt.Key_PageUp: bytes = "\x1b[5~"; break
                case Qt.Key_PageDown: bytes = "\x1b[6~"; break
                default: bytes = event.text
                }
            }
            if (bytes.length) { pty.send(bytes); event.accepted = true }
        }
    }

    onWidthChanged: syncSize()
    onHeightChanged: syncSize()
    Component.onCompleted: { keySink.forceActiveFocus(); pty.start(cols(), rows()) }
}
