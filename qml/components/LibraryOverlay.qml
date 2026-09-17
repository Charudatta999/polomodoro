import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro

// Right edge, 520px — mirrors the task drawer on the left (400px). Wider
// because search results and playlist rows need the measure. Clears the
// 64px now-playing strip; both drawers may be open at once.
Drawer {
    id: root
    edge: Qt.RightEdge
    width: SettingsController.libraryOverlayWidth
    interactive: true
    // Overlay.modal + a transparent ApplicationWindow makes Qt/Hyprland
    // composite the whole surface as alpha. The dim stays "on" after close.
    modal: false
    dim: false

    enter: Transition { NumberAnimation { property: "position"; to: 1; duration: Theme.dEnter; easing.type: Easing.OutQuint } }
    exit:  Transition { NumberAnimation { property: "position"; to: 0; duration: Theme.dEnter; easing.type: Easing.OutQuint } }

    background: Rectangle {
        color: Qt.rgba(0.055, 0.059, 0.075, 0.96)
        border.color: Theme.panelBorder
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.xl
        spacing: Theme.lg

        RowLayout {
            Layout.fillWidth: true
            Text { text: "Music"; font.family: Theme.fontFamily; font.pixelSize: 19; font.weight: Font.DemiBold; color: Theme.textPrimary }
            Item { Layout.fillWidth: true }
            IconButton { glyph: "close"; onClicked: root.close() }
        }

        // Native field, Web API results — no page to type into any more.
        PoloTextField {
            id: search
            Layout.fillWidth: true
            placeholder: "Search Spotify"
            enabled: SpotifyWebApi.authState === "linked" && !SpotifyWebApi.offline
            onTextChanged: debounce.restart()
            Timer {
                id: debounce
                interval: 320
                onTriggered: if (search.text.length > 1) SpotifyWebApi.search(search.text)
            }
        }

        // Now-playing hero — SHARP art. The app background behind this overlay
        // is already the same image blurred; blurring twice just makes mud.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.lg
            visible: MprisController.available

            Rectangle {
                Layout.preferredWidth: 148
                Layout.preferredHeight: 148
                radius: Theme.rRow
                color: Theme.surfaceRaised
                clip: true
                Image {
                    anchors.fill: parent
                    source: MprisController.artUrl
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    sourceSize: Qt.size(296, 296)   // sharp on HiDPI
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 7
                Text {
                    text: MprisController.isSpotifyPlayer ? "NOW PLAYING" : "NOW PLAYING · LOCAL"
                    font.family: Theme.monoFamily; font.pixelSize: 11; font.letterSpacing: 1.8; color: Theme.accent
                }
                Text {
                    Layout.fillWidth: true
                    text: MprisController.title
                    font.family: Theme.fontFamily; font.pixelSize: 21; font.weight: Font.DemiBold
                    color: Theme.textPrimary
                    wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: MprisController.artist + (MprisController.album ? " · " + MprisController.album : "")
                    font.family: Theme.fontFamily; font.pixelSize: 14; color: Theme.textDim
                    elide: Text.ElideRight
                }
            }
        }

        Text {
            text: search.text.length > 1 ? "RESULTS" : "YOUR PLAYLISTS"
            font.family: Theme.monoFamily; font.pixelSize: 11; font.letterSpacing: 1.8
            color: Theme.textFaint
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.sm
            model: search.text.length > 1 ? SpotifyWebApi.results : SpotifyWebApi.playlists
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                width: ListView.view.width
                implicitHeight: 52
                radius: Theme.rRow
                color: hov.hovered ? Theme.surfaceRaised : Theme.surface
                border.width: 1
                border.color: Theme.line
                Behavior on color { ColorAnimation { duration: Theme.dStandard } }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.md
                    spacing: Theme.md
                    Rectangle {
                        Layout.preferredWidth: 34
                        Layout.preferredHeight: 34
                        radius: 5
                        color: Theme.surfaceRaised
                        clip: true
                        Image { anchors.fill: parent; source: modelData.artUrl; fillMode: Image.PreserveAspectCrop; asynchronous: true }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text { text: modelData.name; font.family: Theme.fontFamily; font.pixelSize: 14; color: Theme.textPrimary; elide: Text.ElideRight; Layout.fillWidth: true }
                        Text { text: modelData.subtitle; font.family: Theme.monoFamily; font.pixelSize: 11; color: Theme.textFaint; elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                    IconButton { glyph: "play"; small: true; onClicked: SpotifyWebApi.play(modelData.uri) }
                }
                HoverHandler { id: hov }
            }

            // States 02 and 03 gate the library only — transport stays live.
            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width - Theme.xl
                spacing: Theme.md
                visible: SpotifyWebApi.authState !== "linked" || SpotifyWebApi.offline

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    font.family: Theme.fontFamily; font.pixelSize: 14; font.weight: Font.Medium
                    color: Theme.textPrimary
                    text: SpotifyWebApi.offline ? "Offline" : "Sign in for playlists"
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    font.family: Theme.fontFamily; font.pixelSize: 11; color: Theme.textDim
                    text: SpotifyWebApi.offline
                          ? "Playback controls still work — only the library needs a connection."
                          : "spotifyd login plays on this machine. Playlists and search need the playlist account in Settings → Music."
                }
                PillButton {
                    Layout.alignment: Qt.AlignHCenter
                    visible: !SpotifyWebApi.offline
                    label: SettingsController.spotifyClientId.length > 0 ? "Sign in" : "Add a Client ID in Settings"
                    primary: SettingsController.spotifyClientId.length > 0
                    onClicked: {
                        if (SettingsController.spotifyClientId.length > 0)
                            SpotifyWebApi.beginPkce()
                        else
                            root.close()
                    }
                }
            }
        }
    }
}
