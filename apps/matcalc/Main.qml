import QtQuick
import QtQuick.Controls
import Materialized as M3

ApplicationWindow {
    id: root
    width: 420
    height: 720
    minimumWidth: 360; minimumHeight: 600
    visible: true
    title: qsTr("Calculator")
    color: M3.M3Theme.color("background")

    Column {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16
        M3.M3TopAppBar { width: parent.width; headline: qsTr("Calculator") }
        Item {
            width: parent.width; height: 130
            M3.M3Label {
                anchors.right: parent.right; anchors.top: parent.top
                width: parent.width
                text: calculator.expression.length ? calculator.expression : "0"
                scale: "displayMedium"
                horizontalAlignment: Text.AlignRight
                wrapMode: Text.WrapAnywhere
            }
            M3.M3Label {
                anchors.right: parent.right; anchors.bottom: parent.bottom
                text: calculator.error.length ? calculator.error : calculator.result
                scale: calculator.error.length ? "bodyMedium" : "titleLarge"
                textColor: calculator.error.length ? M3.M3Theme.color("error") : M3.M3Theme.color("onSurfaceVariant")
            }
        }
        Grid {
            width: parent.width
            columns: 4
            spacing: 12
            Repeater {
                model: [
                    { label: "C", action: "clear", kind: "tonal" },
                    { label: "( )", action: "paren", kind: "tonal" },
                    { label: "%", action: "percent", kind: "tonal" },
                    { label: "÷", action: "÷", kind: "accent" },
                    { label: "7", action: "7" }, { label: "8", action: "8" }, { label: "9", action: "9" }, { label: "×", action: "×", kind: "accent" },
                    { label: "4", action: "4" }, { label: "5", action: "5" }, { label: "6", action: "6" }, { label: "-", action: "-", kind: "accent" },
                    { label: "1", action: "1" }, { label: "2", action: "2" }, { label: "3", action: "3" }, { label: "+", action: "+", kind: "accent" },
                    { label: "0", action: "0" }, { label: ".", action: "." }, { label: "⌫", action: "back" }, { label: "=", action: "equals", kind: "primary" }
                ]
                delegate: M3.M3Button {
                    width: (parent.width - parent.spacing * 3) / 4
                    height: 64
                    text: modelData.label
                    containerColor: modelData.kind === "primary" ? M3.M3Theme.color("primary") : modelData.kind === "accent" ? M3.M3Theme.color("secondaryContainer") : M3.M3Theme.color("surfaceContainerHighest")
                    contentColor: modelData.kind === "primary" ? M3.M3Theme.color("onPrimary") : M3.M3Theme.color("onSurface")
                    onClicked: {
                        if (modelData.action === "clear") calculator.clear()
                        else if (modelData.action === "equals") calculator.evaluate()
                        else if (modelData.action === "back") calculator.backspace()
                        else if (modelData.action === "percent") calculator.percent()
                        else if (modelData.action === "paren") {
                            const open = (calculator.expression.match(/\(/g) || []).length
                            const close = (calculator.expression.match(/\)/g) || []).length
                            calculator.input(close < open && /\d$/.test(calculator.expression) ? ")" : "(")
                        } else calculator.input(modelData.action)
                    }
                }
            }
        }
        M3.M3Label { text: qsTr("History"); scale: "labelLarge"; textColor: M3.M3Theme.color("onSurfaceVariant") }
        ListView {
            width: parent.width; height: 120
            model: calculator.history
            delegate: M3.M3Label { text: modelData; scale: "bodyMedium" }
        }
    }
}
