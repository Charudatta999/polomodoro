import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro
import "../components"

Item {
    id: root
    objectName: "mainWindow"

    function toggleTasks() { Window.window.toggleTaskDrawer() }
    function toggleMusic() { Window.window.toggleMusic() }
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
                anchors.margins: Theme.lg
                spacing: Theme.xl

                WindowChrome {
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    Layout.preferredHeight: 38
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
                        id: dialHost
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Qt.rgba(0.078, 0.086, 0.106, Theme.panelOpacity)
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.rPanel
                        clip: true

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: Theme.xl + 2

                            TimerDial {
                                Layout.alignment: Qt.AlignHCenter
                                diameter: Math.min(280, Math.max(140, Math.min(dialHost.width, dialHost.height) - 110))
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
            onLibraryRequested: Window.window.toggleMusic()
        }
    }
}
