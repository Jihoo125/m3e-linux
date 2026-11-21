import QtQuick
import Materialized as M3

Rectangle {
    property bool vertical: false
    property int inset: 0
    width: vertical ? 1 : parent.width
    height: vertical ? parent.height : 1
    color: M3.M3Theme.color("outlineVariant")
    anchors.leftMargin: vertical ? 0 : inset
    anchors.topMargin: vertical ? inset : 0
}
