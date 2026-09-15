import QtQuick
import Polomodoro

// Marquee only when the text overflows: 2s dwell, 34px/s, 2s dwell, reset.
// Never a continuous loop — it would pull the eye all session.
Item {
    id: root
    clip: true
    implicitHeight: label.implicitHeight

    readonly property bool overflowing: label.implicitWidth > root.width

    Text {
        id: label
        text: SpotifyController.nowPlayingLabel.length > 0
              ? "♪ " + SpotifyController.nowPlayingLabel
              : "♪ —"
        font.family: Theme.fontFamily
        font.pixelSize: 11.5
        color: Theme.textDim
        elide: root.overflowing ? Text.ElideNone : Text.ElideRight
        width: root.overflowing ? implicitWidth : root.width

        SequentialAnimation on x {
            running: root.overflowing
            loops: Animation.Infinite
            PauseAnimation { duration: 2000 }
            NumberAnimation {
                to: root.width - label.implicitWidth
                duration: Math.max(1, (label.implicitWidth - root.width) / 34) * 1000
            }
            PauseAnimation { duration: 2000 }
            PropertyAction { value: 0 }
        }
    }
}
