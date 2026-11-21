import QtQuick
import Materialized as M3

Text {
    property string scale: "bodyMedium"
    property bool emphasized: false
    property color textColor: M3.M3Theme.color("onSurface")
    color: textColor
    font: emphasized ? M3.M3Theme.fontEmphasized(scale) : M3.M3Theme.font(scale)
    elide: Text.ElideRight
    wrapMode: Text.NoWrap
    renderType: Text.NativeRendering
}
