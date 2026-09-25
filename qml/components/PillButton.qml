import QtQuick
import Polomodoro

Rectangle {
    id: root
    property string label: ""
    property bool primary: false
    property bool destructive: false
    property bool selected: false
    property bool small: false
    signal clicked()

    implicitHeight: small ? 26 : 40
    implicitWidth: Math.max(small ? 72 : 96, text.implicitWidth + (small ? Theme.sm * 2 + 2 : Theme.xl - 4) * 2)
    radius: Theme.rPill
    color: primary ? (hover.hovered ? Theme.accentHover : Theme.accent)
         : hover.hovered ? Theme.line : Theme.surfaceRaised
    border.width: primary ? 0 : 1
    border.color: selected ? Theme.accent : Theme.line
    scale: press.pressed ? 0.98 : 1.0
    opacity: enabled ? 1 : 0.45
    Behavior on color { ColorAnimation { duration: Theme.dStandard } }
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

    Text {
        id: text
        anchors.centerIn: parent
        text: root.label
        font.family: Theme.fontFamily
        font.pixelSize: root.small ? 10.5 : 13
        font.weight: root.primary ? Font.DemiBold : Font.Normal
        color: root.primary ? Theme.onAccent
             : root.destructive ? Theme.destructive
             : root.selected ? Theme.accent : Theme.textDim
    }

    HoverHandler { id: hover }
    TapHandler { id: press; onTapped: root.clicked() }
}
