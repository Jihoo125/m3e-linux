import QtQuick
import QtQuick.Controls
import Materialized as M3

Popup {
    id: control
    default property alias content: messageRow.data
    property alias text: message.text
    property string actionText: ""
    signal actionTriggered()
    property int duration: 4000
    x: (parent.width - width) / 2
    y: parent.height - height - 24
    width: Math.min(parent.width - 32, 560)
    implicitHeight: Math.max(48, contentItem.implicitHeight + topPadding + bottomPadding)
    padding: 12
    leftPadding: 16; rightPadding: actionText ? 8 : 16
    closePolicy: Popup.NoAutoClose
    Timer { running: control.visible && control.duration > 0; interval: control.duration; onTriggered: control.close() }
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: M3.M3Theme.duration("medium1") } }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: 150 } }
    contentItem: Item {
        implicitHeight: messageRow.height
        Row {
            id: messageRow
            width: parent.width
            M3Label { id: message; width: parent.width - (actionButton.visible ? actionButton.width + 12 : 0); scale: "bodyMedium"; textColor: M3.M3Theme.color("inverseOnSurface"); wrapMode: Text.WordWrap }
            M3TextButton { id: actionButton; text: control.actionText; visible: !!text; contentColor: M3.M3Theme.color("inversePrimary"); onClicked: { control.actionTriggered(); control.close(); } }
        }
    }
    background: Rectangle { radius: M3.M3Theme.radius("small"); color: M3.M3Theme.color("inverseSurface") }
}
