import QtQuick
import QtQuick.Controls
import Materialized as M3

ApplicationWindow {
    id: root
    width: 640; height: 520
    visible: true
    title: qsTr("Screenshot")
    color: M3.M3Theme.color("background")

    Column {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16
        M3.M3TopAppBar { width: parent.width; headline: qsTr("Screenshot"); subtitle: shot.status }
        M3.M3Button {
            text: shot.busy ? qsTr("Capturing…") : qsTr("Capture screen")
            enabled: !shot.busy
            onClicked: shot.capture()
        }
        Rectangle {
            width: parent.width
            height: parent.height - 160
            radius: M3.M3Theme.radius("large")
            color: M3.M3Theme.color("surfaceContainerLow")
            Image {
                anchors.fill: parent
                anchors.margins: 12
                source: shot.lastPath.length ? "file://" + shot.lastPath : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
            M3.M3Label {
                anchors.centerIn: parent
                visible: !shot.lastPath.length
                text: qsTr("Captures use the compositor's screencopy protocol")
                scale: "bodyMedium"
                textColor: M3.M3Theme.color("onSurfaceVariant")
            }
        }
        Row {
            spacing: 12
            M3.M3Button { text: qsTr("Open folder"); onClicked: shot.openFolder() }
        }
    }
}
