pragma Singleton
import QtQuick

QtObject {
    readonly property color bg: "#1a1a2e"
    readonly property color panel: "#CC1a1a2e"
    readonly property color accent: "#7c5cff"
    readonly property color accent2: "#00d4aa"
    readonly property color text: "#f0f0f5"
    readonly property color textMuted: "#a0a0b8"
    readonly property int radius: 12
    readonly property font fontMono: Qt.font({ family: "monospace", pixelSize: 14 })
}
