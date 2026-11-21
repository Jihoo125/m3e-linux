import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Materialized as M3

ApplicationWindow {
    id: root
    width: 1100; height: 780
    visible: true
    title: qsTr("Settings")
    color: M3.M3Theme.color("background")
    property int page: 0
    Row {
        anchors.fill: parent
        Rectangle {
            width: 260; height: parent.height
            color: M3.M3Theme.color("surfaceContainerLow")
            Column {
                anchors.fill: parent; anchors.margins: 16; spacing: 8
                M3.M3Label { text: qsTr("Settings"); scale: "headlineSmall"; bottomPadding: 24 }
                Repeater {
                    model: [qsTr("Appearance"), qsTr("Network"), qsTr("System"), qsTr("Storage"), qsTr("Accessibility"), qsTr("About")]
                    delegate: M3.M3ListItem {
                        width: parent.width
                        text: modelData
                        selected: root.page === index
                        onClicked: root.page = index
                    }
                }
            }
        }
        Rectangle {
            width: parent.width - 260; height: parent.height
            color: M3.M3Theme.color("background")
            StackLayout {
                anchors.fill: parent
                currentIndex: root.page
                Column {
                    anchors.fill: parent; anchors.margins: 32; spacing: 24
                    M3.M3Label { text: qsTr("Appearance"); scale: "headlineMedium" }
                    M3.M3Label { text: qsTr("A desktop that adapts to you."); scale: "bodyLarge"; textColor: M3.M3Theme.color("onSurfaceVariant") }
                    M3.M3Switch { text: qsTr("Dark theme"); checked: M3.M3Theme.dark; onToggled: settings.setDarkMode(checked) }
                    M3.M3Label { text: qsTr("Color style"); scale: "titleMedium" }
                    Flow {
                        width: parent.width; spacing: 8
                        Repeater {
                            model: [
                                { id: "expressive", label: qsTr("Expressive") },
                                { id: "tonal-spot", label: qsTr("Tonal spot") },
                                { id: "vibrant", label: qsTr("Vibrant") },
                                { id: "neutral", label: qsTr("Neutral") },
                                { id: "fidelity", label: qsTr("Fidelity") },
                                { id: "content", label: qsTr("Content") },
                                { id: "monochrome", label: qsTr("Monochrome") },
                                { id: "rainbow", label: qsTr("Rainbow") },
                                { id: "fruit-salad", label: qsTr("Fruit salad") }
                            ]
                            delegate: M3.M3Chip {
                                text: modelData.label
                                selected: M3.M3Theme.variant === modelData.id
                                onClicked: settings.setVariant(modelData.id)
                            }
                        }
                    }
                    M3.M3Label { text: qsTr("Accent color"); scale: "titleMedium" }
                    Row { spacing: 12
                        Repeater { model: ["#6750A4", "#31555C", "#736454", "#A23E48", "#406A4D"]
                            delegate: Rectangle { width: 40; height: 40; radius: 20; color: modelData; MouseArea { anchors.fill: parent; onClicked: settings.setAccent(modelData) } }
                        }
                    }
                    M3.M3TextField { id: wallpaperPath; width: parent.width; label: qsTr("Wallpaper path"); placeholderText: "/usr/share/backgrounds/materialized/materialized-dawn.png" }
                    M3.M3Button { text: qsTr("Apply wallpaper"); enabled: wallpaperPath.text.length > 0; onClicked: settings.setWallpaper(wallpaperPath.text) }
                }
                Column {
                    anchors.fill: parent; anchors.margins: 32; spacing: 24
                    M3.M3Label { text: qsTr("Network"); scale: "headlineMedium" }
                    M3.M3Label { text: settings.networkStatus; scale: "bodyLarge" }
                    M3.M3Label { text: settings.networkAvailable ? qsTr("Wi-Fi and wired connections are managed by NetworkManager.") : qsTr("Install NetworkManager to configure Wi-Fi and network connections."); scale: "bodyMedium"; wrapMode: Text.WordWrap; width: parent.width }
                }
                Column {
                    anchors.fill: parent; anchors.margins: 32; spacing: 24
                    M3.M3Label { text: qsTr("System"); scale: "headlineMedium" }
                    M3.M3TextField { id: hostnameField; width: parent.width; label: qsTr("Hostname"); text: settings.hostname }
                    M3.M3Button { text: qsTr("Set hostname"); onClicked: settings.setHostname(hostnameField.text) }
                    M3.M3TextField { id: timezoneField; width: parent.width; label: qsTr("Time zone"); text: settings.timezone; placeholderText: "Asia/Seoul" }
                    M3.M3Button { text: qsTr("Set time zone"); onClicked: settings.setTimezone(timezoneField.text) }
                    M3.M3TextField { id: keyboardField; width: parent.width; label: qsTr("Keyboard layout"); text: settings.keyboardLayout }
                    M3.M3Button { text: qsTr("Set keyboard layout"); onClicked: settings.setKeyboardLayout(keyboardField.text) }
                }
                Column {
                    anchors.fill: parent; anchors.margins: 32; spacing: 24
                    M3.M3Label { text: qsTr("Storage"); scale: "headlineMedium" }
                    M3.M3Label { text: settings.storageSummary; scale: "titleLarge" }
                    M3.M3Label { text: qsTr("The system uses ext4 by default. Btrfs can be selected during installation."); scale: "bodyMedium"; wrapMode: Text.WordWrap; width: parent.width }
                }
                Column {
                    anchors.fill: parent; anchors.margins: 32; spacing: 24
                    M3.M3Label { text: qsTr("Accessibility"); scale: "headlineMedium" }
                    M3.M3Label { text: qsTr("Semantic colors are generated with contrast-aware tones. Keyboard navigation is supported by all Material controls."); scale: "bodyLarge"; wrapMode: Text.WordWrap; width: parent.width }
                    M3.M3Label { text: qsTr("Use Tab to move between controls and Space to activate them."); scale: "bodyMedium"; textColor: M3.M3Theme.color("onSurfaceVariant") }
                }
                Column {
                    anchors.fill: parent; anchors.margins: 32; spacing: 24
                    M3.M3Label { text: qsTr("About"); scale: "headlineMedium" }
                    M3.M3Label { text: qsTr("Materialized Linux 0.2.1"); scale: "titleLarge" }
                    M3.M3Label { text: settings.operatingSystem; scale: "bodyLarge" }
                    M3.M3Label { text: qsTr("Linux %1").arg(settings.kernel); scale: "bodyMedium" }
                    M3.M3Label { text: qsTr("Material 3 desktop built with Qt 6 and wlroots."); scale: "bodyMedium" }
                }
            }
        }
    }
}
