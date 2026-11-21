import QtQuick
import Materialized as M3

Item {
    id: control
    default property alias content: row.data
    property int currentIndex: 0
    property bool multiSelection: false
    property var selectedIndexes: []
    implicitHeight: 40
    implicitWidth: row.implicitWidth
    Row { id: row; spacing: 0; anchors.fill: parent }
}
