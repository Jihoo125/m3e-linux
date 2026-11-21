import QtQuick
import Materialized as M3

M3Button {
    property string size: "medium"
    property bool extended: text.length > 0
    implicitWidth: extended ? Math.max(80, contentItem.implicitWidth + leftPadding + rightPadding) : (size === "small" ? 40 : size === "large" ? 96 : 56)
    implicitHeight: size === "small" ? 40 : size === "large" ? 96 : 56
    shape: size === "small" ? M3.M3Theme.radius("medium") : M3.M3Theme.radius("large")
    containerColor: M3.M3Theme.color("primaryContainer")
    contentColor: M3.M3Theme.color("onPrimaryContainer")
}
