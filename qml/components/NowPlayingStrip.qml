import QtQuick
import QtQuick.Layouts
import Polomodoro

// The ONLY always-visible playback surface, so it must CONTROL, not report.
// 64px, full window width, pinned to the bottom of MainWindow.
Rectangle {
    id: root
    signal libraryRequested()

    implicitHeight: 64
    color: Qt.rgba(0.055, 0.059, 0.075, 0.96)
    border.color: Theme.panelBorder
    border.width: 1

    // Three independent failures — never collapsed into one message.
    readonly property bool noPlayer: !MprisController.available
    readonly property bool notLinked: SpotifyWebApi.authState === "none"
    readonly property bool offline: SpotifyWebApi.offline

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.xl
        anchors.rightMargin: Theme.xl
        spacing: Theme.lg + 2
        opacity: root.offline ? 0.62 : 1

        // Art, or a placeholder tile when there is nothing to show.
        Rectangle {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            radius: 5
            color: Theme.surfaceRaised
            border.width: root.noPlayer ? 1 : 0
            border.color: Theme.line
            clip: true

            Image {
                anchors.fill: parent
                source: MprisController.artUrl      // mpris:artUrl — a clean CDN URL
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                visible: source !== ""
            }
            Text {
                anchors.centerIn: parent
                visible: MprisController.artUrl === ""
                text: "♪"
                font.pixelSize: 15
                color: Theme.textFaint
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 190
            spacing: 1
            Text {
                Layout.fillWidth: true
                font.family: Theme.fontFamily
                font.pixelSize: 13
                font.weight: Font.Medium
                color: Theme.textPrimary
                elide: Text.ElideRight
                text: root.noPlayer ? "No player running"
                    : (MprisController.title !== "" ? MprisController.title
                       : root.notLinked ? "Connect your Spotify account"
                       : "No track")
            }
            Text {
                Layout.fillWidth: true
                font.family: Theme.fontFamily
                font.pixelSize: 11
                color: root.offline ? Theme.overflow : Theme.textDim
                elide: Text.ElideRight
                text: root.noPlayer
                    ? (!SpotifydManager.binaryFound ? "Install spotifyd (see README), or play from any Spotify app"
                       : SpotifydManager.authenticating ? "Finish spotifyd sign-in in the browser"
                       : !SpotifydManager.credentialsPresent ? "Settings → Music → Sign in to spotifyd (needed to play here)"
                       : SpotifydManager.running ? "Waiting for Spotify to list “" + SettingsController.spotifyDeviceName + "”"
                       : "Starting spotifyd…")
                    : MprisController.artist !== "" ? MprisController.artist
                    : root.notLinked ? "Needed for playlists, search and devices"
                    : root.offline ? "Offline — showing last known state"
                    : ""
            }
        }

        // Two different logins — never collapse them. Web API (playlists)
        // vs spotifyd (this machine can actually play).
        PillButton {
            visible: root.notLinked && !root.noPlayer && MprisController.isSpotifyPlayer
            label: "Sign in"
            primary: true
            small: true
            onClicked: SpotifyWebApi.beginPkce()
        }
        PillButton {
            visible: root.noPlayer && SpotifydManager.binaryFound && !SpotifydManager.credentialsPresent
            label: SpotifydManager.authenticating ? "Waiting…" : "Sign in to spotifyd"
            primary: true
            small: true
            enabled: !SpotifydManager.authenticating
            onClicked: SpotifydManager.authenticate()
        }

        // Transport needs no account and no network — MPRIS is a local bus.
        RowLayout {
            spacing: Theme.sm
            enabled: !root.noPlayer
            opacity: enabled ? 1 : 0.35
            IconButton { glyph: "prev"; small: true; onClicked: MprisController.previous() }
            IconButton {
                glyph: MprisController.isPlaying ? "pause" : "play"
                filled: true
                onClicked: MprisController.togglePlayPause()
            }
            IconButton { glyph: "next"; small: true; onClicked: MprisController.next() }
        }

        // Seekable, not a read-out.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.md - 1
            visible: !root.noPlayer
            Text {
                text: MprisController.positionLabel
                font.family: Theme.monoFamily; font.pixelSize: 11; color: Theme.textFaint
            }
            ProgressTrough {
                id: scrub
                Layout.fillWidth: true
                implicitHeight: 4
                ratio: MprisController.positionRatio
                fill: Theme.accent
                TapHandler {
                    onTapped: function(e) { MprisController.seekToRatio(e.position.x / scrub.width) }
                }
            }
            Text {
                text: MprisController.durationLabel
                font.family: Theme.monoFamily; font.pixelSize: 11; color: Theme.textFaint
            }
        }

        RowLayout {
            spacing: Theme.md
            IconButton {
                glyph: "device"
                tooltip: SpotifyWebApi.currentDeviceName || "Choose player or device"
                enabled: MprisController.players.length > 0 || (!root.notLinked && !root.offline)
                onClicked: devices.open()
            }
            IconButton { glyph: "tasks"; tooltip: "Library (Ctrl+M)"; onClicked: root.libraryRequested() }
        }
    }

    DevicePopover { id: devices; y: -implicitHeight - 8 }
}
