import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Polomodoro

// No Save button: every control writes through SettingsController to the
// settings table immediately, inside a transaction. Durations apply from the
// NEXT phase — changing work length at 12:03 must not jump the clock.
Popup {
    id: root
    parent: Overlay.overlay
    modal: true
    dim: false
    width: 640
    height: 460
    padding: 0
    anchors.centerIn: parent

    background: Rectangle {
        color: Qt.rgba(0.078, 0.086, 0.106, 0.90)
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.rPanel
    }

    readonly property var sections: ["Timer", "Appearance", "Tasks", "Notifications", "Music", "Data"]
    property int section: 0

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
        Rectangle {
            Layout.preferredWidth: 168
            Layout.fillHeight: true
            color: "transparent"
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.line }
            Column {
                anchors.fill: parent
                anchors.margins: Theme.lg
                spacing: 3
                Repeater {
                    model: root.sections
                    Rectangle {
                        width: parent.width
                        height: 32
                        radius: 7
                        color: root.section === index ? Theme.surfaceRaised : "transparent"
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            x: Theme.md - 1
                            text: modelData
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                            font.weight: root.section === index ? Font.Medium : Font.Normal
                            color: root.section === index ? Theme.textPrimary : Theme.textDim
                        }
                        TapHandler { onTapped: root.section = index }
                    }
                }
            }
        }

        // Pane
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: pane.implicitHeight + Theme.lg * 2
            clip: true

            Column {
                id: pane
                x: Theme.lg + 2
                y: Theme.lg + 2
                width: parent.width - (Theme.lg + 2) * 2
                spacing: Theme.md - 1

                Text { text: root.sections[root.section]; font.family: Theme.fontFamily; font.pixelSize: 16; font.weight: Font.DemiBold; color: Theme.textPrimary }

                // Label, settings key beneath in faint mono, control right-
                // aligned. Exposing the key is deliberate: the owner of a
                // single-user app may well open the database by hand.
                SettingRow {
                    visible: root.section === 0
                    label: "Work length"; key: "pomodoroWorkMs"
                    SpinBox { from: 1; to: 180; value: SettingsController.workMinutes; onValueModified: SettingsController.workMinutes = value }
                }
                SettingRow {
                    visible: root.section === 0
                    label: "Short break"; key: "pomodoroShortBreakMs"
                    SpinBox { from: 1; to: 60; value: SettingsController.shortBreakMinutes; onValueModified: SettingsController.shortBreakMinutes = value }
                }
                SettingRow {
                    visible: root.section === 0
                    label: "Long break"; key: "pomodoroLongBreakMs · every 4th cycle"
                    SpinBox { from: 1; to: 120; value: SettingsController.longBreakMinutes; onValueModified: SettingsController.longBreakMinutes = value }
                }

                SettingRow {
                    visible: root.section === 1
                    label: "Accent colour"; key: "accentMode"
                    PoloSegmented {
                        options: ["Auto from image", "Manual"]
                        currentIndex: SettingsController.accentMode === "auto" ? 0 : 1
                        onPicked: SettingsController.accentMode = (index === 0 ? "auto" : "manual")
                    }
                }
                // Manual reveals four curated FULL palettes — not a free
                // picker, so break/overflow/muted stay coherent.
                Row {
                    visible: root.section === 1 && SettingsController.accentMode === "manual"
                    spacing: 7
                    Repeater {
                        model: SettingsController.palettePresets
                        Rectangle {
                            width: 54; height: 26; radius: 6
                            color: modelData.accent
                            border.width: SettingsController.accentManualPalette === modelData.id ? 2 : 0
                            border.color: Theme.textPrimary
                            TapHandler { onTapped: SettingsController.accentManualPalette = modelData.id }
                        }
                    }
                }
                SettingRow {
                    visible: root.section === 1
                    label: "Background source"; key: "backgroundSource"
                    BackgroundSourceToggle {}
                }
                SettingRow {
                    visible: root.section === 1
                    label: "Wallpaper shows"; key: "backgroundPlacement"
                    PoloSegmented {
                        options: ["Frame", "Middle", "Both"]
                        currentIndex: SettingsController.backgroundPlacement === "frame" ? 0
                                    : SettingsController.backgroundPlacement === "middle" ? 1 : 2
                        onPicked: SettingsController.backgroundPlacement =
                            index === 0 ? "frame" : index === 1 ? "middle" : "both"
                    }
                }
                SettingRow {
                    visible: root.section === 1
                    label: "Wallpaper folder"; key: "backgroundUserPath"
                    PillButton {
                        label: "Choose folder"
                        small: true
                        onClicked: wallpaperFolder.open()
                    }
                }
                SettingRow {
                    visible: root.section === 1
                    label: "Rotation interval"; key: "backgroundRotationSec"
                    Slider { from: 60; to: 1800; stepSize: 30; value: SettingsController.rotationSec; onMoved: SettingsController.rotationSec = value }
                }
                SettingRow {
                    visible: root.section === 1
                    label: "Always on top"; key: "alwaysOnTop · Ctrl+Shift+T"
                    PoloToggle { checked: SettingsController.alwaysOnTop; onToggled: SettingsController.alwaysOnTop = checked }
                }
                SettingRow {
                    visible: root.section === 2
                    label: "Progress counts"; key: "targetProgressBasis"
                    PoloSegmented {
                        options: ["Active time", "Logged work"]
                        currentIndex: SettingsController.progressBasis === "active" ? 0 : 1
                        onPicked: SettingsController.progressBasis = (index === 0 ? "active" : "logged")
                    }
                }
                SettingRow {
                    visible: root.section === 3
                    label: "Target reached"; key: "notifyOnTargetReached"
                    PoloToggle {
                        checked: SettingsController.notifyOnTargetReached
                        onToggled: SettingsController.notifyOnTargetReached = checked
                    }
                }
                SettingRow {
                    visible: root.section === 3
                    label: "Task started"; key: "notifyOnTaskStart"
                    PoloToggle {
                        checked: SettingsController.notifyOnTaskStart
                        onToggled: SettingsController.notifyOnTaskStart = checked
                    }
                }
                SettingRow {
                    visible: root.section === 3
                    label: "End date approaching"; key: "notifyOnEndDateApproaching"
                    PoloToggle {
                        checked: SettingsController.notifyOnEndDateApproaching
                        onToggled: SettingsController.notifyOnEndDateApproaching = checked
                    }
                }
                // Client id is the only piece of PKCE config that lives here:
                // it is public by design (no secret ships). The refresh token
                // this produces never touches this settings table — see
                // SpotifyWebApi, which stores it in the system keyring.
                SettingRow {
                    visible: root.section === 4
                    label: "Playlist account"; key: "authState · Web API PKCE, not spotifyd"
                    RowLayout {
                        spacing: Theme.md
                        Text {
                            text: SpotifyWebApi.authState === "linked" ? "Signed in"
                                : SpotifyWebApi.authState === "linking" ? "Signing in…"
                                : SpotifyWebApi.authState === "expired" ? "Session expired"
                                : "Not signed in"
                            font.family: Theme.fontFamily; font.pixelSize: 13
                            color: SpotifyWebApi.authState === "linked" ? Theme.accent : Theme.textPrimary
                        }
                        PillButton {
                            label: SpotifyWebApi.authState === "linked" ? "Sign out"
                                 : SpotifyWebApi.authState === "linking" ? "Waiting…"
                                 : "Sign in"
                            small: true
                            primary: SpotifyWebApi.authState !== "linked"
                            enabled: SettingsController.spotifyClientId.length > 0
                                     && SpotifyWebApi.authState !== "linking"
                            onClicked: SpotifyWebApi.authState === "linked" ? SpotifyWebApi.signOut() : SpotifyWebApi.beginPkce()
                        }
                    }
                }
                SettingRow {
                    visible: root.section === 4
                    label: "Client ID"; key: "spotifyClientId"
                    PoloTextField {
                        implicitWidth: 220
                        text: SettingsController.spotifyClientId
                        placeholder: "From developer.spotify.com"
                        onEditingFinished: SettingsController.spotifyClientId = text
                        onTextChanged: if (text !== SettingsController.spotifyClientId)
                            SettingsController.spotifyClientId = text
                    }
                }
                SettingRow {
                    visible: root.section === 4
                    label: "Auto-start spotifyd"; key: "spotifyAutoLaunch"
                    PoloToggle {
                        checked: SettingsController.spotifyAutoLaunch
                        onToggled: SettingsController.spotifyAutoLaunch = checked
                    }
                }
                // Own device name, own config/cache dir — never the same
                // spotifyd instance as one the user already runs themselves.
                SettingRow {
                    visible: root.section === 4
                    label: "This machine's device name"; key: "spotifyDeviceName"
                    PoloTextField {
                        implicitWidth: 260
                        text: SettingsController.spotifyDeviceName
                        placeholder: "Polomodoro"
                        onEditingFinished: SettingsController.spotifyDeviceName = text
                    }
                }
                SettingRow {
                    visible: root.section === 4
                    label: "spotifyd login"; key: "librespot OAuth — not the Client ID above"
                    RowLayout {
                        spacing: Theme.md
                        Text {
                            text: SpotifydManager.authenticating ? "Signing in…"
                                : SpotifydManager.credentialsPresent ? "Signed in"
                                : "Not signed in"
                            font.family: Theme.fontFamily; font.pixelSize: 13
                            color: SpotifydManager.credentialsPresent ? Theme.accent : Theme.textDim
                        }
                        PillButton {
                            label: SpotifydManager.authenticating ? "Waiting…"
                                 : SpotifydManager.credentialsPresent ? "Sign in again"
                                 : "Sign in"
                            small: true
                            primary: !SpotifydManager.credentialsPresent
                            enabled: SpotifydManager.binaryFound && !SpotifydManager.authenticating
                            onClicked: SpotifydManager.authenticate()
                        }
                    }
                }
                SettingRow {
                    visible: root.section === 4
                    label: "spotifyd status"; key: "running"
                    RowLayout {
                        spacing: Theme.md
                        Text {
                            text: !SpotifydManager.binaryFound ? "Not installed"
                                : SpotifydManager.running ? "Running" : "Stopped"
                            font.family: Theme.fontFamily; font.pixelSize: 13
                            color: SpotifydManager.running ? Theme.accent : Theme.textDim
                        }
                        PillButton {
                            label: "Restart"
                            small: true
                            visible: SpotifydManager.binaryFound
                            onClicked: SpotifydManager.restart()
                        }
                    }
                }
                SettingRow {
                    visible: root.section === 4
                    label: "Now-playing strip"; key: "nowPlayingStripVisible"
                    PoloToggle {
                        checked: SettingsController.nowPlayingStripVisible
                        onToggled: SettingsController.nowPlayingStripVisible = checked
                    }
                }

                SettingRow {
                    visible: root.section === 5
                    label: "Export sessions"; key: "CSV to ~/Documents"
                    PillButton { label: "Export"; onClicked: SettingsController.exportCsv() }
                }
                Text {
                    visible: root.section === 5 && SettingsController.integrityReport.length > 0
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: SettingsController.integrityReport
                    font.family: Theme.monoFamily; font.pixelSize: 11; color: Theme.textDim
                }
            }
        }
    }

    FolderDialog {
        id: wallpaperFolder
        title: "Choose a wallpaper folder"
        onAccepted: BackgroundController.setUserFolder(selectedFolder)
    }
}
