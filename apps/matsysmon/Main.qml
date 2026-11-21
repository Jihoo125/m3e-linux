import QtQuick
import QtQuick.Controls
import Materialized as M3

ApplicationWindow {
    id: root
    width: 1080; height: 760
    visible: true
    title: qsTr("System Monitor")
    color: M3.M3Theme.color("background")
    Column {
        anchors.fill: parent
        M3.M3TopAppBar { width: parent.width; headline: qsTr("System Monitor"); subtitle: qsTr("Uptime %1").arg(processes.uptime) }
        Row {
            spacing: 16
            M3.M3Card { width: 250; height: 100
                Column { anchors.centerIn: parent
                    M3.M3Label { text: qsTr("CPU usage"); scale: "labelLarge" }
                    M3.M3Label { text: processes.totalCpu.toFixed(1) + "%"; scale: "headlineMedium" }
                }
            }
            M3.M3Card { width: 300; height: 100
                Column { anchors.centerIn: parent
                    M3.M3Label { text: qsTr("Memory"); scale: "labelLarge" }
                    M3.M3Label { text: "%1 / %2 GiB".arg((processes.usedMemory / 1073741824).toFixed(1)).arg((processes.totalMemory / 1073741824).toFixed(1)); scale: "headlineMedium" }
                }
            }
        }
        M3.M3TextField { width: parent.width; label: qsTr("Filter processes"); placeholderText: qsTr("Name"); onTextChanged: processes.filter = text }
        Rectangle { width: parent.width; height: 1; color: M3.M3Theme.color("outlineVariant") }
        ListView {
            width: parent.width; height: root.height - 280
            model: processes
            clip: true
            delegate: Rectangle {
                width: parent.width; height: 52
                color: index % 2 ? "transparent" : M3.M3Theme.color("surfaceContainerLow")
                Row {
                    anchors.fill: parent; anchors.margins: 12; spacing: 16
                    M3.M3Label { text: name; width: 230; scale: "bodyMedium" }
                    M3.M3Label { text: pid; width: 65; scale: "bodyMedium" }
                    M3.M3Label { text: user; width: 100; scale: "bodyMedium" }
                    M3.M3Label { text: cpu.toFixed(1) + "%"; width: 85; scale: "bodyMedium" }
                    M3.M3Label { text: (memory / 1048576).toFixed(1) + " MiB"; width: 110; scale: "bodyMedium" }
                    M3.M3TextButton { text: qsTr("End"); onClicked: processes.terminate(pid) }
                }
            }
        }
    }
}
