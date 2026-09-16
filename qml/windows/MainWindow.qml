import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro
import "../components"

Item {
    id: root
    objectName: "mainWindow"

    function toggleTasks() { Window.window.toggleTaskDrawer() }
    function toggleMusic() { if (library.opened) library.close(); else library.open() }
    function newTask() { Window.window.newTask() }
    function openSettings() { Window.window.openSettings() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Everything that used to sit directly under the margined layout now
        // lives in this Item, so NowPlayingStrip below it can run full-width,
        // flush to the window edge, per its own spec ("64px, full window
        // width, pinned to the bottom of MainWindow").
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.xxl
                spacing: Theme.xl

                WindowChrome {
                    Layout.fillWidth: true
                    onTasksClicked: root.toggleTasks()
                    onSettingsClicked: root.openSettings()
                }

                ActiveTasksBar {
                    Layout.fillWidth: true
                    visible: TaskController.activeCount > 0
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Theme.xl

                    // Frosted timer panel
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Qt.rgba(0.078, 0.086, 0.106, Theme.panelOpacity)
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.rPanel

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: Theme.xl + 2

                            TimerDial {
                                Layout.alignment: Qt.AlignHCenter
                                diameter: Math.max(200, Math.min(280, root.height * 0.48))
                            }

                            ModeSwitcher { Layout.alignment: Qt.AlignHCenter }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: TimerController.cycleLabel
                                font.family: Theme.monoFamily
                                font.pixelSize: Theme.fNumeric
                                color: Theme.textDim
                            }
                        }
                    }

                    // Day column: fixed width, collapses behind a header toggle below
                    // 1040px rather than shrinking (labels collide at ~300px).
                    DayTimeline {
                        Layout.preferredWidth: 392
                        Layout.fillHeight: true
                        visible: SettingsController.showDayTimeline && Window.window.width >= 1040
                    }
                }
            }
        }

        // The only always-visible playback surface — persistent, not a
        // drawer, so it does not overlay/scrim (R4 governs the two drawers
        // below, not this).
        NowPlayingStrip {
            id: strip
            Layout.fillWidth: true
            visible: SettingsController.nowPlayingStripVisible
            onLibraryRequested: library.open()
        }
    }

    // Right-edge overlay, per R4: overlays with a scrim, never docks. Parented
    // to Overlay.overlay (like TaskMenuDrawer) so it spans the real window,
    // not just this Item's local bounds. Stops above the strip rather than
    // covering it — both drawers may be open at the same time.
    LibraryOverlay {
        id: library
        parent: Overlay.overlay
        height: root.height - (strip.visible ? strip.height : 0)
    }
}
