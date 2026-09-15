import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    spacing: 6

    property bool spotifyOpen: false
    signal spotifyToggled()

    ChromeButton {
        text: "Tasks"
        onClicked: taskController.setMenuOpen(true)
    }

    Item { Layout.fillWidth: true }

    ChromeButton {
        text: backgroundController.backgroundSource === "wallpaper" ? "Wallpaper" : "Album art"
        onClicked: backgroundController.cycleBackgroundSource()
        ToolTip.visible: hovered
        ToolTip.text: "Switch background source"
    }

    ChromeButton {
        text: "Music"
        checked: root.spotifyOpen
        onClicked: root.spotifyToggled()
        ToolTip.visible: hovered
        ToolTip.text: "Show or hide Spotify panel (Ctrl+M)"
    }

    ChromeButton {
        text: "Pin"
        checked: windowLayout.alwaysOnTop
        onClicked: {
            windowLayout.alwaysOnTop = !windowLayout.alwaysOnTop
            settingsController.alwaysOnTop = windowLayout.alwaysOnTop
        }
        ToolTip.visible: hovered
        ToolTip.text: "Always on top"
    }

    Rectangle {
        width: 1
        height: 24
        color: "#33ffffff"
    }

    ChromeButton {
        text: "Full"
        checked: windowLayout.viewMode === "expanded"
        onClicked: windowLayout.setViewMode("expanded")
    }
    ChromeButton {
        text: "Bar"
        checked: windowLayout.viewMode === "bar"
        onClicked: windowLayout.setViewMode("bar")
    }
    ChromeButton {
        text: "Mini"
        checked: windowLayout.viewMode === "compact"
        onClicked: windowLayout.setViewMode("compact")
    }
}
