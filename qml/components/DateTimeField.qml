import QtQuick
import QtQuick.Controls
import Polomodoro

TextField {
    id: root
    property var utcValue: null
    property string placeholder: ""
    placeholderText: placeholder
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fBodyS
    color: Theme.textPrimary
    padding: Theme.sm
    background: Rectangle {
        radius: Theme.rRow
        color: Theme.surfaceRaised
        border.color: Theme.line
        border.width: 1
    }
}
