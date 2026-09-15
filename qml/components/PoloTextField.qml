import QtQuick
import QtQuick.Controls
import Polomodoro

TextField {
    id: root
    property string placeholder: ""
    placeholderText: placeholder
    placeholderTextColor: Theme.textFaint
    color: Theme.textPrimary
    font.family: Theme.fontFamily
    font.pixelSize: 14
    implicitHeight: 38
    leftPadding: Theme.md
    rightPadding: Theme.md
    selectByMouse: true

    background: Rectangle {
        radius: Theme.rRow
        color: Theme.surfaceRaised
        // Border-only focus: a 2px ring in a derived colour clashes at some
        // hues, and a glow reads as an error state.
        border.width: 1
        border.color: root.activeFocus ? Theme.accent : Theme.line
        Behavior on border.color { ColorAnimation { duration: Theme.dStandard } }
    }
}
