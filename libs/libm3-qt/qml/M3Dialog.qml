import QtQuick
import QtQuick.Controls
import Materialized as M3

Dialog {
    id: control
    property string icon: ""
    property string headline: title
    property alias bodyText: bodyLabel.text
    default property alias content: contentArea.data
    property Item affirmativeButton: acceptButton
    property Item negativeButton: null
    modal: true
    focus: true
    padding: 24
    implicitWidth: Math.max(280, Math.min(560, contentItem.implicitWidth + leftPadding + rightPadding))
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
    anchors.centerIn: Overlay.overlay
    standardButtons: Dialog.NoButton
    enter: Transition { ParallelAnimation { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: M3.M3Theme.duration("medium2"); easing.type: Easing.Bezier; easing.bezierCurve: M3.M3Theme.easing("emphasizedDecelerate") } NumberAnimation { property: "scale"; from: 0.8; to: 1; duration: M3.M3Theme.duration("medium2"); easing.type: Easing.Bezier; easing.bezierCurve: M3.M3Theme.easing("emphasizedDecelerate") } } }
    exit: Transition { ParallelAnimation { NumberAnimation { property: "opacity"; to: 0; duration: M3.M3Theme.duration("short3") } NumberAnimation { property: "scale"; to: 0.9; duration: M3.M3Theme.duration("short3") } } }

    contentItem: Column {
        width: Math.min(parent.width, 512)
        spacing: 16
        M3Label { text: control.headline; scale: "headlineSmall"; visible: !!text; width: parent.width; wrapMode: Text.WordWrap }
        Item { id: contentArea; width: parent.width; height: childrenRect.height > 0 ? childrenRect.height : 0; visible: children.length > 0 }
        M3Label { id: bodyLabel; scale: "bodyMedium"; textColor: M3.M3Theme.color("onSurfaceVariant"); width: parent.width; wrapMode: Text.WordWrap; visible: !!text }
        Row {
            id: actions
            layoutDirection: Qt.RightToLeft
            anchors.right: parent.right
            spacing: 8
            M3TextButton { id: acceptButton; text: qsTr("OK"); onClicked: control.accept() }
        }
    }
    background: Rectangle {
        radius: M3.M3Theme.radius("extraLarge")
        color: M3.M3Theme.color("surfaceContainerHigh")
        border.width: control.visualFocus ? 2 : 0
        border.color: M3.M3Theme.color("primary")
    }
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.4) }
}
