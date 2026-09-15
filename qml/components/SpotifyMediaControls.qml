import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    spacing: 4

    ChromeButton {
        text: "Prev"
        implicitWidth: 40
        implicitHeight: 28
        onClicked: spotifyBridge.previous()
    }
    ChromeButton {
        text: spotifyBridge.isPlaying ? "Pause" : "Play"
        implicitWidth: 44
        implicitHeight: 28
        accentColor: "#1db954"
        onClicked: spotifyBridge.togglePlayPause()
    }
    ChromeButton {
        text: "Next"
        implicitWidth: 40
        implicitHeight: 28
        onClicked: spotifyBridge.next()
    }
}
