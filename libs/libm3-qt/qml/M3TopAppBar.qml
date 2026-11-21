import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Materialized as M3

ToolBar {
    id: control
    property string headline: ""
    property string subtitle: ""
    property string leadingIcon: ""
    property string type: "small" // small | medium | large | center-aligned
    default property alias actions: actionRow.data
    signal leadingTriggered()
    property bool scrolled: false
    implicitHeight: type === "large" ? 112 : type === "medium" ? 104 : 64
    padding: 16
    leftPadding: leadingIcon ? 8 : 16
    rightPadding: 8

    contentItem: Item {
        implicitHeight: control.height - control.topPadding - control.bottomPadding
        Row {
            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            M3IconButton { visible: !!control.leadingIcon; icon.source: control.leadingIcon; onClicked: control.leadingTriggered() }
            Column {
                anchors.verticalCenter: control.type === "small" || control.type === "center-aligned" ? parent.verticalCenter : undefined
                anchors.bottom: control.type === "large" || control.type === "medium" ? parent.bottom : undefined
                anchors.bottomMargin: 8
                M3Label { text: control.headline; scale: control.scrolled ? "titleLarge" : control.type === "large" ? "headlineLarge" : control.type === "medium" ? "headlineMedium" : "titleLarge"; wrapMode: Text.NoWrap }
                M3Label { text: control.subtitle; scale: "bodyMedium"; textColor: M3.M3Theme.color("onSurfaceVariant"); visible: !!text }
            }
        }
        Row { id: actionRow; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 0 }
    }
    background: Rectangle { color: control.scrolled ? M3.M3Theme.color("surfaceContainer") : M3.M3Theme.color("surface") }
}
