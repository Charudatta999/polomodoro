import QtQuick
import Polomodoro

// Marquee only when the text overflows: 2s dwell, 34px/s, 2s dwell, reset.
// Never a continuous loop — it would pull the eye all session.
Item {
    id: root
    clip: true
    implicitHeight: label.implicitHeight

    readonly property string trackText: MprisController.nowPlayingLabel
    readonly property bool hasTrack: trackText.length > 0
    readonly property bool overflowing: hasTrack && label.implicitWidth > root.width

    Text {
        id: label
        text: hasTrack ? ("♪ " + root.trackText) : "♪ —"
        font.family: Theme.fontFamily
        font.pixelSize: 12
        color: Theme.textDim
        elide: root.overflowing ? Text.ElideNone : Text.ElideRight
        width: root.overflowing ? implicitWidth : root.width

        SequentialAnimation {
            id: scrollAnim
            running: root.overflowing
            loops: Animation.Infinite
            PauseAnimation { duration: 2000 }
            NumberAnimation {
                target: label
                property: "x"
                to: root.width - label.implicitWidth
                duration: Math.max(1, (label.implicitWidth - root.width) / 34) * 1000
            }
            PauseAnimation { duration: 2000 }
            PropertyAction { target: label; property: "x"; value: 0 }
        }
    }

    Connections {
        target: MprisController
        function onPlaybackChanged() {
            label.x = 0
            scrollAnim.stop()
            if (root.overflowing)
                scrollAnim.start()
        }
    }

    onWidthChanged: {
        label.x = 0
        scrollAnim.stop()
        if (root.overflowing)
            scrollAnim.start()
    }
}
