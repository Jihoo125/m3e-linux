import QtQuick
import QtQuick.Controls
import Materialized as M3

Menu {
    id: control
    padding: 4
    implicitWidth: Math.max(112, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
    transformOrigin: Menu.TopLeft
    enter: Transition { ParallelAnimation { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: M3.M3Theme.duration("short4") } NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: M3.M3Theme.duration("short4"); easing.type: Easing.Bezier; easing.bezierCurve: M3.M3Theme.easing("emphasizedDecelerate") } } }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: 150 } }
    delegate: M3MenuItem {}
    background: Rectangle {
        implicitWidth: 112; implicitHeight: 48
        radius: M3.M3Theme.radius("small")
        color: M3.M3Theme.color("surfaceContainer")
        border.width: 0
    }
    contentItem: ListView {
        implicitHeight: contentHeight
        model: control.contentModel
        interactive: height < contentHeight
        clip: true
        keyNavigationEnabled: true
        currentIndex: control.currentIndex
        ScrollIndicator.vertical: ScrollIndicator {}
    }
}
