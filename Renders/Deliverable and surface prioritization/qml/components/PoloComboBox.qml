import QtQuick
import QtQuick.Controls
import Polomodoro

ComboBox {
    id: root
    property string placeholder: ""
    readonly property string currentId: currentIndex >= 0 && model[currentIndex] ? model[currentIndex].id : ""
    textRole: "title"
    implicitHeight: 38
    font.family: Theme.fontFamily
    font.pixelSize: 13

    background: Rectangle {
        radius: Theme.rRow
        color: Theme.surfaceRaised
        border.width: 1
        border.color: root.activeFocus ? Theme.accent : Theme.line
    }
    contentItem: Text {
        leftPadding: Theme.md
        text: root.currentIndex < 0 ? root.placeholder : root.displayText
        font: root.font
        color: root.currentIndex < 0 ? Theme.textDim : Theme.textPrimary
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
