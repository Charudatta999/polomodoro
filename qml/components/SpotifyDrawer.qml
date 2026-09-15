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
    property int drawerWidth: expanded ? 340 : 34
    color: Qt.rgba(0.055, 0.059, 0.075, 0.94)
    border.color: Theme.panelBorder
    border.width: 1
    radius: Theme.rPanel
    clip: true

    Layout.preferredWidth: drawerWidth
    Layout.fillHeight: true
    Behavior on drawerWidth {
        NumberAnimation { duration: Theme.dEnter; easing.type: Easing.OutQuint }
    }

    readonly property int chromeHeight: 44
    readonly property int panelWidth: 340

    function attachPlayer() {
        SpotifyController.attach(web)
        web.runJavaScript(
            "(function(){" +
            "let m=document.querySelector('meta[name=viewport]');" +
            "if(!m){m=document.createElement('meta');m.name='viewport';document.head.appendChild(m);}" +
            "m.content='width=device-width, initial-scale=1, maximum-scale=1';" +
            "if (document.getElementById('polo-spotify-css')) return;" +
            "const s=document.createElement('style');" +
            "s.id='polo-spotify-css';" +
            "s.textContent='" +
            "#onetrust-banner-sdk,#onetrust-consent-sdk,.ot-sdk-container," +
            "[aria-label=\"Cookie banner\"],div[id*=\"cookie-policy\"]{display:none!important}" +
            "html,body{min-width:0!important}" +
            "';" +
            "document.documentElement.appendChild(s);" +
            "})()")
    }

    ColumnLayout {
        x: 0
        y: 0
        width: root.panelWidth
        height: parent.height
        spacing: 0
        enabled: root.expanded

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.chromeHeight
            Layout.leftMargin: Theme.md + 2
            Layout.rightMargin: Theme.md + 2
            Text { text: "Music"; font.family: Theme.fontFamily; font.pixelSize: 14; font.weight: Font.DemiBold; color: Theme.textPrimary }
            Item { Layout.fillWidth: true }
            IconButton { glyph: "close"; onClicked: root.expanded = false }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }

        Item {
            id: webHost
            Layout.preferredWidth: root.panelWidth
            Layout.fillHeight: true
            Layout.bottomMargin: Theme.rPanel
            clip: true

            WebEngineView {
                id: web
                anchors.fill: parent
                url: "https://open.spotify.com"
                zoomFactor: 1.0
                backgroundColor: "#121212"

                settings.showScrollBars: false
                profile: SpotifyWebProfile

                Component.onCompleted: root.attachPlayer()
                onLoadingChanged: function(req) {
                    if (req.status === WebEngineView.LoadSucceededStatus)
                        root.attachPlayer()
                }
            }
        }

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

    Item {
        anchors.fill: parent
        visible: !root.expanded
        z: 1
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.055, 0.059, 0.075, 0.94)
            radius: Theme.rPanel
        }
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
}
