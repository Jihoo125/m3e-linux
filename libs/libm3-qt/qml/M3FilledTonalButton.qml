import Materialized as M3

M3Button {
    containerColor: enabled ? M3.M3Theme.color("secondaryContainer") : Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, 0.12)
    contentColor: enabled ? M3.M3Theme.color("onSecondaryContainer") : Qt.rgba(M3.M3Theme.color("onSurface").r, M3.M3Theme.color("onSurface").g, M3.M3Theme.color("onSurface").b, 0.38)
}
