import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine

Rectangle {
    id: root
    radius: 12
    color: "#ee12121f"
    border.color: "#33ffffff"
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "Spotify"
                color: "#f0f0f5"
                font.pixelSize: 14
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Label {
                text: "Premium required for playback"
                color: "#8888a0"
                font.pixelSize: 10
            }
        }

        WebEngineView {
            id: webView
            Layout.fillWidth: true
            Layout.fillHeight: true
            url: "https://open.spotify.com"

            profile: WebEngineProfile {
                storageName: "polomodoro-spotify"
                offTheRecord: false
            }

            Component.onCompleted: {
                spotifyBridge.attachWebPage(webView.page)
                spotifyBridge.startPolling(2000)
            }
        }
    }
}
