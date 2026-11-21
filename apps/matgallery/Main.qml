import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Materialized as M3

Item {
    id: root
    width: 1120
    height: 840

    Rectangle { anchors.fill: parent; color: M3.M3Theme.color("background") }

    M3.M3TopAppBar {
        id: appBar
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        headline: qsTr("Materialized Design System")
        subtitle: qsTr("Revision %1 · live Material 3 roles").arg(M3.M3Theme.revision)
    }

    Flickable {
        id: flick
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: appBar.bottom; anchors.bottom: parent.bottom
        clip: true
        contentWidth: width
        contentHeight: galleryColumn.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick

        Column {
            id: galleryColumn
            width: flick.width
            padding: 24
            spacing: 24

            M3.M3Label { text: qsTr("Component gallery"); scale: "displaySmall" }
            M3.M3Label { text: qsTr("Controls are bound to semantic roles, shared motion and shape tokens. Change the wallpaper or color scheme while this window is open."); scale: "bodyLarge"; wrapMode: Text.WordWrap; width: galleryColumn.width - galleryColumn.padding * 2 }

            M3.M3Card { width: galleryColumn.width - galleryColumn.padding * 2
                Column {
                    anchors.left: parent.left; anchors.right: parent.right
                    spacing: 16
                    M3.M3Label { text: qsTr("Buttons and common actions"); scale: "titleLarge" }
                    Flow {
                        width: parent.width; spacing: 12
                        M3.M3Button { text: qsTr("Filled") }
                        M3.M3FilledTonalButton { text: qsTr("Tonal") }
                        M3.M3OutlinedButton { text: qsTr("Outlined") }
                        M3.M3TextButton { text: qsTr("Text") }
                        M3.M3ElevatedButton { text: qsTr("Elevated") }
                        M3.M3Button { text: qsTr("Disabled"); enabled: false }
                        M3.M3Button { text: qsTr("Loading"); busy: true }
                        M3.M3IconButton { text: "★" }
                        M3.M3IconButton { text: "♥"; selected: true }
                        M3.M3FAB { text: "+" }
                    }
                }
            }

            M3.M3Card { width: galleryColumn.width - galleryColumn.padding * 2; tone: "outlined"
                Column {
                    anchors.left: parent.left; anchors.right: parent.right
                    spacing: 16
                    M3.M3Label { text: qsTr("Selection controls and input"); scale: "titleLarge" }
                    Grid {
                        width: parent.width; columns: width > 800 ? 2 : 1; columnSpacing: 32; rowSpacing: 24
                        Column {
                            width: (parent.width - parent.columnSpacing * (parent.columns - 1)) / parent.columns
                            spacing: 16
                            M3.M3Switch { checked: true }
                            M3.M3Switch { }
                            M3.M3Checkbox { text: qsTr("Automatic updates"); checked: true }
                            M3.M3RadioButton { text: qsTr("Balanced power"); checked: true }
                            M3.M3RadioButton { text: qsTr("Power saver") }
                        }
                        Column {
                            width: (parent.width - parent.columnSpacing * (parent.columns - 1)) / parent.columns
                            spacing: 16
                            M3.M3Label { text: qsTr("Input and feedback"); scale: "titleLarge" }
                            M3.M3TextField { label: qsTr("Hostname"); text: "materialized"; supportingText: qsTr("Visible on your local network"); width: parent.width }
                            M3.M3TextField { label: qsTr("Password"); echoMode: TextInput.Password; outlined: false; width: parent.width }
                            M3.M3TextField { label: qsTr("Username"); text: "\""; errorText: qsTr("This name contains unsupported characters"); width: parent.width }
                            M3.M3Slider { from: 0; to: 100; value: 65; showValue: true; width: parent.width }
                            M3.M3Progress { value: 0.64; width: parent.width }
                            Row { spacing: 16; M3.M3CircularProgress { running: true; width: 32; height: 32 } M3.M3CircularProgress { value: 0.64; width: 32; height: 32 } M3.M3Badge { text: "12" } }
                        }
                    }
                }
            }

            M3.M3Card { width: galleryColumn.width - galleryColumn.padding * 2
                Column {
                    anchors.left: parent.left; anchors.right: parent.right
                    spacing: 16
                    M3.M3Label { text: qsTr("Navigation and menus"); scale: "titleLarge" }
                    Flow {
                        width: parent.width; spacing: 12
                        M3.M3Chip { text: qsTr("All files") }
                        M3.M3Chip { text: qsTr("Images"); kind: "filter"; checked: true }
                        M3.M3Chip { text: qsTr("Documents"); kind: "filter" }
                    }
                    M3.M3ListItem { text: qsTr("Materialized Linux"); supportingText: qsTr("A calm, coherent desktop"); trailingText: qsTr("Now"); width: parent.width }
                    M3.M3Divider { width: parent.width }
                    M3.M3ListItem { text: qsTr("Notification settings"); supportingText: qsTr("Choose what deserves your attention"); width: parent.width }
                    M3.M3Button { text: qsTr("Open dialog"); onClicked: exampleDialog.open() }
                    M3.M3Button { text: qsTr("Show snackbar"); containerColor: M3.M3Theme.color("secondaryContainer"); contentColor: M3.M3Theme.color("onSecondaryContainer"); onClicked: snackbar.open() }
                }
            }

            M3.M3Label { text: qsTr("Accessible semantic colors"); scale: "titleLarge" }
            Grid {
                width: galleryColumn.width - galleryColumn.padding * 2
                columns: width < 700 ? 4 : 8
                columnSpacing: 8; rowSpacing: 8
                Repeater {
                    model: ListModel {
                        ListElement { roleName: "primary" } ListElement { roleName: "onPrimary" }
                        ListElement { roleName: "primaryContainer" } ListElement { roleName: "onPrimaryContainer" }
                        ListElement { roleName: "secondary" } ListElement { roleName: "onSecondary" }
                        ListElement { roleName: "secondaryContainer" } ListElement { roleName: "onSecondaryContainer" }
                        ListElement { roleName: "tertiary" } ListElement { roleName: "onTertiary" }
                        ListElement { roleName: "tertiaryContainer" } ListElement { roleName: "onTertiaryContainer" }
                        ListElement { roleName: "surface" } ListElement { roleName: "onSurface" }
                        ListElement { roleName: "surfaceContainer" } ListElement { roleName: "surfaceVariant" }
                        ListElement { roleName: "onSurfaceVariant" } ListElement { roleName: "error" }
                        ListElement { roleName: "errorContainer" } ListElement { roleName: "onErrorContainer" }
                        ListElement { roleName: "outline" } ListElement { roleName: "inverseSurface" }
                        ListElement { roleName: "inverseOnSurface" } ListElement { roleName: "inversePrimary" }
                    }
                    delegate: ColorSwatch {
                        required property string roleName
                        role: roleName
                        width: (parent.width - parent.columnSpacing * (parent.columns - 1)) / parent.columns
                    }
                }
            }
            Item { width: 1; height: 24 }
        }
    }

    M3.M3Dialog {
        id: exampleDialog
        headline: qsTr("A comfortable dialog")
        bodyText: qsTr("Dialogs use a tonal surface, 28 dp corners and an emphasized deceleration curve. Focus returns to the trigger when they close.")
        M3.M3TextButton { text: qsTr("Cancel"); onClicked: exampleDialog.close() }
    }
    M3.M3Snackbar { id: snackbar; parent: root; text: qsTr("Your preferences have been saved"); actionText: qsTr("Undo") }
}
