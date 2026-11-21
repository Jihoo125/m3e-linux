import QtQuick
import QtQuick.Controls
import Materialized as M3

TextField {
    id: control
    property string label: ""
    property string supportingText: ""
    property string errorText: ""
    property bool outlined: true
    property string leadingIcon: ""
    property string trailingIcon: ""
    readonly property bool error: errorText.length > 0
    implicitWidth: 210
    implicitHeight: 56
    leftPadding: leadingIcon ? 40 : 16
    rightPadding: trailingIcon ? 40 : 16
    topPadding: label && !outlined ? 22 : 16
    bottomPadding: 12
    verticalAlignment: TextInput.AlignVCenter
    color: error ? M3.M3Theme.color("error") : M3.M3Theme.color("onSurface")
    selectionColor: M3.M3Theme.color("primaryContainer")
    selectedTextColor: M3.M3Theme.color("onPrimaryContainer")
    placeholderTextColor: M3.M3Theme.color("onSurfaceVariant")
    font: M3.M3Theme.font("bodyLarge")
    hoverEnabled: true

    background: Item {
        implicitHeight: 56
        Rectangle {
            anchors.fill: parent
            radius: control.outlined ? M3.M3Theme.radius("small") : 4
            color: control.outlined ? "transparent" : M3.M3Theme.color("surfaceVariant")
            border.width: control.activeFocus ? 2 : 1
            border.color: control.error ? M3.M3Theme.color("error") : control.activeFocus ? M3.M3Theme.color("primary") : M3.M3Theme.color("outline")
        }
        Rectangle {
            x: 12; y: -8
            width: floatingLabel.implicitWidth + 8
            height: control.activeFocus || control.text.length > 0 ? floatingLabel.implicitHeight : 0
            color: M3.M3Theme.color("surface")
            visible: control.outlined && !!control.label && height > 0
            z: 1
        }
        M3Label {
            id: floatingLabel
            x: 16
            z: 2
            y: control.activeFocus || control.text.length > 0 ? -8 : 17
            text: control.label
            scale: control.activeFocus || control.text.length > 0 ? "bodySmall" : "bodyLarge"
            textColor: control.error ? M3.M3Theme.color("error") : control.activeFocus ? M3.M3Theme.color("primary") : M3.M3Theme.color("onSurfaceVariant")
            visible: control.outlined && !!control.label
            padding: 0
            Behavior on y { NumberAnimation { duration: M3.M3Theme.duration("short3") } }
            Behavior on font.pixelSize { NumberAnimation { duration: M3.M3Theme.duration("short3") } }
        }
        M3Label {
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.top: parent.top; anchors.topMargin: 6
            text: control.label
            visible: !control.outlined && !!control.label
            scale: "bodySmall"
            textColor: control.activeFocus ? M3.M3Theme.color("primary") : M3.M3Theme.color("onSurfaceVariant")
        }
    }

    M3Label {
        id: helper
        anchors.left: parent.left; anchors.leftMargin: 16
        anchors.top: parent.bottom; anchors.topMargin: 4
        text: control.errorText || control.supportingText
        visible: !!text
        scale: "bodySmall"
        textColor: control.error ? M3.M3Theme.color("error") : M3.M3Theme.color("onSurfaceVariant")
    }
}
