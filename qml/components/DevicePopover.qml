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
        anchors.fill: parent
        spacing: 2

        Text {
            Layout.margins: Theme.sm
            text: "PLAYING ON"
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
            text: SpotifydManager.running
                ? "No devices yet. Open Spotify anywhere and select “" + SettingsController.spotifyDeviceName + "” from the Connect device list."
                : "No devices found. Open Spotify anywhere, or check Settings → Music if spotifyd isn't running."
            wrapMode: Text.WordWrap
            font.family: Theme.fontFamily; font.pixelSize: 11; color: Theme.textDim
        }
    }
}
