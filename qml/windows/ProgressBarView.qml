import QtQuick
import QtQuick.Layouts
import Polomodoro
import "../components"

Rectangle {
    id: root
    color: Qt.rgba(0.102, 0.102, 0.180, 0.92)
    border.color: Qt.rgba(0.929, 0.937, 0.953, 0.12)
    border.width: 1
    radius: Theme.rRow

    property bool dropdownOpen: SettingsController.barDropdownExpanded
    readonly property int barHeight: 48
    readonly property int openHeight: Math.min(240, barHeight + dropdown.contentHeight)

    readonly property bool isActive: Window.window && Window.window.viewMode === 1

    // Only for the dropdown resizing while already in bar mode. Entering bar
    // mode is main.qml's job (WindowLayoutManager's stored geometry), and this
    // used to also fire then via onIsActiveChanged — isActive flips synchronously
    // when Window.window.viewMode changes, which is before main.qml's
    // Qt.callLater(_applyWindowGeometryNow) has run. That raced two resize
    // requests back to back on the same frameless Wayland surface (an
    // immediate height-only one from here, then main.qml's width+height one),
    // and the second reliably lost its width component — bar mode kept the
    // previous mode's width. Wayland xdg_toplevel resizes are a request/ack
    // cycle, not a fire-and-forget property write; issuing two in the same
    // event loop turn is not safe to assume both land.
    function syncBarHeight() {
        if (!isActive || !Window.window)
            return
        Window.window.height = dropdownOpen ? openHeight : barHeight
    }

    onDropdownOpenChanged: {
        SettingsController.barDropdownExpanded = dropdownOpen
        syncBarHeight()
    }

    // Background drag. A MouseArea declared first sits below every control, so
    // the transport, progress bar and buttons still get their own events and
    // only bare background starts a move — "draggable by their background".
    //
    // A MouseArea rather than a DragHandler: Wayland validates startSystemMove
    // against the input serial of the event that triggered it, and a handler
    // that only activates after the drag threshold presents a stale serial,
    // which the compositor quietly refuses.
    MouseArea {
        id: backgroundDrag
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        // On first motion rather than on press, so a plain click still lands
        // (PiP double-click expands) while the serial is still current.
        onPositionChanged: if (pressed && Window.window) Window.window.startSystemMove()
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
            // SpotifyMediaControls was deleted with the WebEngine pivot; the
            // updated reference bundle's ProgressBarView.qml still names it
            // (stale — the pivot only reached NowPlayingLabel there), so this
            // is the compact MPRIS transport in its place, matching the
            // prev/playpause/next set in NowPlayingStrip.
            RowLayout {
                Layout.preferredWidth: 96
                spacing: Theme.xs + 2
                enabled: MprisController.available
                opacity: enabled ? 1 : 0.4
                IconButton { glyph: "prev"; small: true; onClicked: MprisController.previous() }
                IconButton {
                    glyph: MprisController.isPlaying ? "pause" : "play"
                    filled: true
                    onClicked: MprisController.togglePlayPause()
                }
                IconButton { glyph: "next"; small: true; onClicked: MprisController.next() }
            }
            Divider {}

            OverallProgressBar {
                Layout.fillWidth: true
                expanded: root.dropdownOpen
                onToggleRequested: root.dropdownOpen = !root.dropdownOpen
            }

            Divider {}

            NowPlayingLabel { Layout.preferredWidth: 170 }

            IconButton { glyph: "tasks"; onClicked: Window.window.toggleTaskDrawer() }
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
    focus: true
}
