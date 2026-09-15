import QtQuick
import QtQuick.Layouts
import Polomodoro
import "../components"

// Bar mode — 800x48, always-on-top by default. The WINDOW height animates,
// not an inner clip: a frameless top-most bar must not leave a transparent
// dead zone over other windows.
Rectangle {
    id: root
    color: Qt.rgba(0.102, 0.102, 0.180, 0.92)
    border.color: Qt.rgba(0.929, 0.937, 0.953, 0.12)
    border.width: 1
    radius: Theme.rRow

    property bool dropdownOpen: SettingsController.barDropdownExpanded
    readonly property int barHeight: 48
    readonly property int openHeight: Math.min(240, barHeight + dropdown.contentHeight)

    onDropdownOpenChanged: {
        SettingsController.barDropdownExpanded = dropdownOpen
        Window.window.height = dropdownOpen ? openHeight : barHeight
    }
    Behavior on height { NumberAnimation { duration: Theme.dEnter; easing.type: Easing.OutQuint } }

    DragHandler {
        target: null
        // Background only — dragging must not fight the progress bar or timer.
        onActiveChanged: if (active) Window.window.startSystemMove()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.barHeight
            spacing: Theme.md + 2

            Item { implicitWidth: Theme.md }

            Text {
                Layout.preferredWidth: 72
                text: TimerController.formattedTime
                font.family: Theme.monoFamily
                font.pixelSize: 18
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
                color: Theme.textPrimary
                TapHandler { onTapped: TimerController.toggle() }
            }

            Divider {}
            SpotifyMediaControls { Layout.preferredWidth: 96 }
            Divider {}

            OverallProgressBar {
                Layout.fillWidth: true
                expanded: root.dropdownOpen
                onToggleRequested: root.dropdownOpen = !root.dropdownOpen
            }

            Divider {}

            NowPlayingLabel { Layout.preferredWidth: 170 }

            IconButton { glyph: "tasks"; onClicked: barTasks.open() }
            IconButton { glyph: "expand"; onClicked: WindowLayoutManager.setMode(0) }

            Item { implicitWidth: Theme.sm }
        }

        SubtaskProgressDropdown {
            id: dropdown
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.dropdownOpen
        }
    }

    Keys.onEscapePressed: root.dropdownOpen = false
    TaskMenuDrawer { id: barTasks; height: 520 }
}
