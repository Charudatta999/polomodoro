import QtQuick
import QtQuick.Controls
import Polomodoro

// 28x28 visual, 36x36 hit area. Desktop pointer, so 44px is not required —
// but the invisible skirt stops mis-clicks between Pause and Stop in a dense
// list. Glyphs are vector paths in resources/icons/, NOT font characters: an
// icon font falls back to a random glyph on a bare Arch install and the media
// triangles are the worst offenders.
Item {
    id: root
    property string glyph: ""
    property bool filled: false
    property bool small: false
    property string tooltip: ""
    signal clicked()

    implicitWidth: small ? 22 : 28
    implicitHeight: implicitWidth

    Rectangle {
        anchors.centerIn: parent
        width: root.implicitWidth
        height: root.implicitHeight
        radius: Theme.rPill
        color: root.filled ? Theme.accent : (hover.hovered ? Theme.line : Theme.surfaceRaised)
        border.width: root.filled ? 0 : 1
        border.color: Theme.line
        Behavior on color { ColorAnimation { duration: Theme.dStandard } }

        Image {
            anchors.centerIn: parent
            width: root.small ? 9 : 11
            height: width
            source: "qrc:/icons/" + root.glyph + ".svg"
            sourceSize: Qt.size(width * 2, height * 2)
        }
    }

    // Invisible 36x36 skirt.
    MouseArea {
        anchors.centerIn: parent
        width: 36; height: 36
        onClicked: root.clicked()
        cursorShape: Qt.PointingHandCursor
    }
    HoverHandler { id: hover }
    ToolTip.visible: root.tooltip.length > 0 && hover.hovered
    ToolTip.text: root.tooltip
}
