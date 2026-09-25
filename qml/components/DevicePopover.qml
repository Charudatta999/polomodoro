import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro

// Replaces the old "Play here" button: that only had a meaning when the local
// player happened to be spotifyd. A device list lets the user pick any target.
Popup {
    id: root
    width: 280
    padding: Theme.sm
    background: Rectangle {
        color: Qt.rgba(0.078, 0.086, 0.106, 0.96)
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.rRow
    }

    ColumnLayout {
        width: root.availableWidth
        spacing: 2

        Text {
            Layout.margins: Theme.sm
            text: "ON THIS MACHINE"
            font.family: Theme.monoFamily
            font.pixelSize: 9
            font.letterSpacing: 1.6
            color: Theme.textFaint
        }

        Repeater {
            model: MprisController.players
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 40
                radius: 6
                readonly property bool current: modelData.current === true
                color: hovMpris.hovered ? Theme.surfaceRaised : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.md
                    anchors.rightMargin: Theme.md
                    spacing: Theme.md
                    Text {
                        text: modelData.spotify ? "♪" : "▣"
                        font.pixelSize: 12
                        color: parent.parent.current ? Theme.accent : Theme.textDim
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text {
                            text: modelData.name
                            font.family: Theme.fontFamily; font.pixelSize: 13
                            color: parent.parent.parent.current ? Theme.accent : Theme.textPrimary
                            elide: Text.ElideRight
                        }
                        Text {
                            text: modelData.spotify ? "Spotify / spotifyd" : "MPRIS"
                            font.family: Theme.monoFamily; font.pixelSize: 10; color: Theme.textFaint
                        }
                    }
                }
                HoverHandler { id: hovMpris }
                TapHandler {
                    onTapped: {
                        MprisController.selectPlayer(modelData.service)
                        root.close()
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.margins: Theme.md
            visible: MprisController.players.length === 0
            text: "No local players on D-Bus yet. Start spotifyd, Spotify, or a browser tab with media."
            wrapMode: Text.WordWrap
            font.family: Theme.fontFamily; font.pixelSize: 11; color: Theme.textDim
        }

        Text {
            Layout.margins: Theme.sm
            Layout.topMargin: Theme.md
            text: "SPOTIFY CONNECT"
            font.family: Theme.monoFamily
            font.pixelSize: 9
            font.letterSpacing: 1.6
            color: Theme.textFaint
        }

        Repeater {
            model: SpotifyWebApi.devices
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 40
                radius: 6
                readonly property bool current: modelData.id === SpotifyWebApi.currentDeviceId
                color: hov.hovered ? Theme.surfaceRaised : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.md
                    anchors.rightMargin: Theme.md
                    spacing: Theme.md
                    Text {
                        text: modelData.isLocal ? "▣" : "▭"
                        font.pixelSize: 12
                        color: parent.parent.current ? Theme.accent : Theme.textDim
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text {
                            text: modelData.name
                            font.family: Theme.fontFamily; font.pixelSize: 13
                            color: parent.parent.parent.current ? Theme.accent : Theme.textPrimary
                            elide: Text.ElideRight
                        }
                        Text {
                            visible: modelData.isLocal
                            text: "This machine"
                            font.family: Theme.monoFamily; font.pixelSize: 10; color: Theme.textFaint
                        }
                    }
                }
                HoverHandler { id: hov }
                // Transport routes to MPRIS for a local target, Web API otherwise.
                TapHandler { onTapped: { SpotifyWebApi.transferTo(modelData.id); root.close() } }
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.margins: Theme.md
            visible: SpotifyWebApi.devices.length === 0
            text: !SpotifydManager.credentialsPresent
                ? "spotifyd has no login yet. Settings → Music → Sign in to spotifyd — this is separate from the playlist account."
                : SpotifydManager.running
                ? "No devices yet. “" + SettingsController.spotifyDeviceName + "” should appear after spotifyd finishes signing in."
                : "No devices found. Open Spotify anywhere, or check Settings → Music if spotifyd isn't running."
            wrapMode: Text.WordWrap
            font.family: Theme.fontFamily; font.pixelSize: 11; color: Theme.textDim
        }
    }
}
