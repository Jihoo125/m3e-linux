import QtQuick
import QtQuick.Controls
import Materialized as M3

Item {
    id: root
    width: 1280
    height: 800

    property bool launcherVisible: false
    property bool quickSettingsVisible: false
    property bool notificationsVisible: false
    property string wallpaper: applicationModel.wallpaper

    Rectangle { anchors.fill: parent; color: M3.M3Theme.color("background") }
    Image {
        id: wallpaperImage
        anchors.fill: parent
        source: root.wallpaper
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        opacity: 0.45
        visible: status === Image.Ready
    }
    Rectangle {
        anchors.fill: parent
        color: M3.M3Theme.color("background")
        opacity: wallpaperImage.status === Image.Ready ? 0.35 : 1.0
    }

    Row {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 24
        anchors.bottomMargin: 24
        spacing: 12
        Repeater {
            model: [
                { name: qsTr("Files"), exec: "matfiles" },
                { name: qsTr("Monitor"), exec: "matsysmon" },
                { name: qsTr("Gallery"), exec: "matgallery" },
                { name: qsTr("Settings"), exec: "matsettings" },
                { name: qsTr("Editor"), exec: "mattext" },
                { name: qsTr("Calculator"), exec: "matcalc" },
                { name: qsTr("Terminal"), exec: "matterminal" },
                { name: qsTr("Images"), exec: "matview" },
                { name: qsTr("Calendar"), exec: "matcal" },
                { name: qsTr("Screenshot"), exec: "matshot" },
                { name: qsTr("Browser"), exec: "matbrowser" }
            ]
            delegate: M3.M3Button {
                text: modelData.name
                containerColor: M3.M3Theme.color("surfaceContainerHigh")
                contentColor: M3.M3Theme.color("onSurface")
                onClicked: applicationModel.launchByName(modelData.exec)
            }
        }
    }

    M3.M3Label {
        anchors.centerIn: parent
        text: qsTr("Materialized Linux")
        scale: "displayMedium"
        textColor: M3.M3Theme.color("onSurface")
        visible: !root.launcherVisible
    }

    Panel {
        id: panel
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        onLauncherRequested: root.launcherVisible = !root.launcherVisible
        onQuickSettingsRequested: root.quickSettingsVisible = !root.quickSettingsVisible
        onNotificationsRequested: root.notificationsVisible = !root.notificationsVisible
    }

    Launcher {
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: panel.bottom; anchors.bottom: parent.bottom
        visible: root.launcherVisible
        onDismissed: root.launcherVisible = false
    }
    QuickSettings {
        anchors.right: parent.right; anchors.top: panel.bottom; anchors.bottom: parent.bottom
        visible: root.quickSettingsVisible
        onDismissed: root.quickSettingsVisible = false
    }
    NotificationCenter {
        anchors.right: parent.right; anchors.top: panel.bottom; anchors.bottom: parent.bottom
        visible: root.notificationsVisible
        onDismissed: root.notificationsVisible = false
    }
}
