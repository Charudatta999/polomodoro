import QtQuick
import Polomodoro

Rectangle {
    id: root
    property bool planned: false
    property bool running: false
    property int overflowMs: 0
    property string title: ""
    property string detail: ""

    radius: Theme.rTrough
    color: planned ? "transparent"
         : running ? "transparent"
         : overflowMs > 0 ? Theme.overflow : Theme.accent
    border.width: planned || running ? 1 : 0
    border.color: planned ? Theme.muted : Qt.alpha(Theme.accent, 0.35)

    // Hatched, open-ended: no bottom edge, because it has no end yet.
    Canvas {
        anchors.fill: parent
        visible: root.running
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = Qt.alpha(Theme.accent, 0.20)
            ctx.lineWidth = 3
            for (var i = -height; i < width; i += 10) {
                ctx.beginPath(); ctx.moveTo(i, height); ctx.lineTo(i + height, 0); ctx.stroke()
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 1
        // Labels are dropped below 34px; the tooltip carries them instead.
        visible: root.height >= 34
        Text {
            text: root.title
            font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold
            color: root.planned || root.running ? Theme.textDim : Theme.onAccent
            elide: Text.ElideRight; width: parent.width
        }
        Text {
            text: root.detail
            font.family: Theme.monoFamily; font.pixelSize: 9
            color: root.planned || root.running ? Theme.textDim : Theme.onAccent
            elide: Text.ElideRight; width: parent.width
        }
    }

    ToolTip.visible: hover.hovered
    ToolTip.text: root.title + " · " + root.detail
    HoverHandler { id: hover }
}
