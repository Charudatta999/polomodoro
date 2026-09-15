import QtQuick
import QtQuick.Layouts
import Polomodoro

// MediaSession first, DOM button click as fallback. Both are fragile; the
// button shows an optimistic state for 600ms then reconciles from the next
// metadata poll, or the DOM path feels broken.
RowLayout {
    id: root
    spacing: Theme.xs + 2
    enabled: SpotifyController.ready
    opacity: enabled ? 1 : 0.4

    property bool optimisticPlaying: SpotifyController.isPlaying
    Timer {
        id: reconcile
        interval: 600
        onTriggered: root.optimisticPlaying = SpotifyController.isPlaying
    }

    IconButton {
        glyph: "prev"; small: true
        tooltip: "Previous (Alt+[)"
        onClicked: SpotifyController.previous()
    }
    IconButton {
        glyph: root.optimisticPlaying ? "pause" : "play"
        filled: true
        tooltip: root.optimisticPlaying ? "Pause" : "Play"
        onClicked: {
            root.optimisticPlaying = !root.optimisticPlaying
            reconcile.restart()
            SpotifyController.togglePlayPause()
        }
    }
    IconButton {
        glyph: "next"; small: true
        tooltip: "Next (Alt+])"
        onClicked: SpotifyController.next()
    }

    // Media keys are frequently grabbed by the compositor before Qt sees
    // them; the Alt+[ / Alt+] pair is a documented fallback, not an alias.
    Shortcut { sequence: StandardKey.MediaPlay; onActivated: SpotifyController.togglePlayPause() }
    Shortcut { sequence: StandardKey.MediaNext; onActivated: SpotifyController.next() }
    Shortcut { sequence: StandardKey.MediaPrevious; onActivated: SpotifyController.previous() }
    Shortcut { sequence: "Alt+]"; onActivated: SpotifyController.next() }
    Shortcut { sequence: "Alt+["; onActivated: SpotifyController.previous() }
}
