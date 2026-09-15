import QtQuick
import Polomodoro

Rectangle {
    id: root
    property bool checked: false
    signal toggled()
    implicitWidth: 34
    implicitHeight: 20
    radius: Theme.rPill
    color: checked ? Theme.accent : Theme.surfaceRaised
    border.width: checked ? 0 : 1
    border.color: Theme.line
    Behavior on color { ColorAnimation { duration: Theme.dStandard } }

    Rectangle {
        width: 14; height: 14; radius: 7
        y: 3
        x: root.checked ? root.width - width - 3 : 3
        color: root.checked ? Theme.bgBase : Theme.textFaint
        Behavior on x { NumberAnimation { duration: Theme.dStandard; easing.type: Easing.OutCubic } }
    }

    TapHandler { onTapped: { root.checked = !root.checked; root.toggled() } }
}
