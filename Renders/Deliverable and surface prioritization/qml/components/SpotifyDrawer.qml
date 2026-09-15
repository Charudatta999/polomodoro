import QtQuick
import QtQuick.Layouts
import QtWebEngine
import Polomodoro

// Pushes the timer panel rather than overlaying it — music is a companion,
// not a modal task. The view is NEVER unloaded, only hidden: art, metadata
// and transport keep working in bar and PiP modes.
Rectangle {
    id: root
    property bool expanded: false
    color: Qt.rgba(0.055, 0.059, 0.075, 0.94)
    border.color: Theme.panelBorder
    border.width: 1
    radius: Theme.rPanel
    clip: true

    // Collapsed edge tab.
    Item {
        anchors.fill: parent
        visible: !root.expanded
        Text {
            anchors.centerIn: parent
            rotation: 90
            text: "MUSIC ◂"
            font.family: Theme.monoFamily
            font.pixelSize: 11
            font.letterSpacing: 1.8
            color: Theme.textDim
        }
        TapHandler { onTapped: root.expanded = true }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: root.expanded

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.leftMargin: Theme.md + 2
            Layout.rightMargin: Theme.md + 2
            Text { text: "Music"; font.family: Theme.fontFamily; font.pixelSize: 13.5; font.weight: Font.DemiBold; color: Theme.textPrimary }
            Item { Layout.fillWidth: true }
            IconButton { glyph: "close"; onClicked: root.expanded = false }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }

        WebEngineView {
            id: web
            Layout.fillWidth: true
            Layout.fillHeight: true
            url: "https://open.spotify.com"

            // Fixed storageName + persistent cookies, or the user
            // re-authenticates on every launch.
            profile: WebEngineProfile {
                storageName: "polomodoro-spotify"
                persistentCookiesPolicy: WebEngineProfile.AllowPersistentCookies
            }

            onLoadingChanged: function(req) {
                if (req.status === WebEngineView.LoadSucceededStatus)
                    SpotifyController.attach(web)
            }
        }

        // Shown instead of a broken player when the web app refuses playback.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            visible: SpotifyController.premiumRequired
            color: Theme.surfaceRaised
            Text {
                anchors.centerIn: parent
                text: "Playback needs Spotify Premium — see README"
                font.family: Theme.fontFamily
                font.pixelSize: 11
                color: Theme.textDim
            }
        }
    }
}
