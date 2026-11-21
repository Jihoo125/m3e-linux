import QtQuick
import Materialized as M3

Item {
    id: control
    property string tone: "filled"
    property int elevationLevel: tone === "elevated" ? 1 : 0
    property int cornerRadius: M3.M3Theme.radius("medium")
    property bool interactive: false
    property bool pressed: false
    property int padding: 16
    default property alias content: contentArea.data
    implicitWidth: contentArea.implicitWidth + padding * 2
    implicitHeight: contentArea.implicitHeight + padding * 2

    Rectangle {
        anchors.fill: parent
        radius: control.cornerRadius
        color: control.tone === "elevated" ? M3.M3Theme.color("surfaceContainerLow")
             : control.tone === "outlined" ? M3.M3Theme.color("surface")
             : M3.M3Theme.color("surfaceContainerHighest")
        border.width: control.tone === "outlined" ? 1 : 0
        border.color: M3.M3Theme.color("outlineVariant")
    }
    Item {
        id: contentArea
        anchors.fill: parent
        anchors.margins: control.padding
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
    }
}
