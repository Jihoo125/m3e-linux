import Materialized as M3

M3Button {
    horizontalInset: 12
    implicitWidth: Math.max(48, contentItem.implicitWidth + leftPadding + rightPadding)
    containerColor: "transparent"
    contentColor: enabled ? M3.M3Theme.color("primary") : Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, 0.38)
}
