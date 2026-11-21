import QtQuick
import QtQuick.Controls
import Materialized as M3

TabBar {
    id: control
    implicitHeight: 80
    spacing: 0
    background: Rectangle { color: M3.M3Theme.color("surface") }
    contentItem: ListView {
        model: control.contentModel
        currentIndex: control.currentIndex
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.AutoFlickIfNeeded
        snapMode: ListView.SnapToItem
        highlightMoveDuration: M3.M3Theme.duration("medium2")
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: 0
        preferredHighlightEnd: width
        interactive: false
    }
}
