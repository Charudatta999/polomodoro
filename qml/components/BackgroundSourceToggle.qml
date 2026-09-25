import QtQuick
import QtQuick.Layouts
import Polomodoro

// Wallpaper | Now playing. Persisted as backgroundSource; Ctrl+B cycles.
Rectangle {
    id: root
    implicitHeight: 38
    implicitWidth: row.implicitWidth + Theme.sm - 2
    radius: Theme.rPill
    color: Qt.rgba(0.078, 0.086, 0.106, Theme.panelOpacity)
    border.color: Theme.panelBorder
    border.width: 1

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 3

        Repeater {
            model: [{ key: "wallpaper", glyph: "wallpaper" }, { key: "spotify", glyph: "art" }]
            Rectangle {
                Layout.preferredHeight: 32
                Layout.preferredWidth: 46
                radius: Theme.rPill
                readonly property bool on: BackgroundController.source === modelData.key
                color: on ? Theme.accent : "transparent"
                Behavior on color { ColorAnimation { duration: Theme.dStandard } }
                Image {
                    anchors.centerIn: parent
                    width: 14
                    height: 14
                    source: "qrc:/icons/" + modelData.glyph + ".svg"
                    sourceSize: Qt.size(28, 28)
                }
                TapHandler { onTapped: BackgroundController.source = modelData.key }
            }
        }
    }
}
