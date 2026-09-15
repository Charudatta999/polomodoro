import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

ApplicationWindow {
    id: root
    visible: true
    color: "#12121f"
    title: "Polomodoro"
    flags: windowLayout.alwaysOnTop ? Qt.Window | Qt.WindowStaysOnTopHint : Qt.Window

    property var geo: windowLayout.geometryForMode(windowLayout.viewMode)
    property bool spotifyOpen: false

    width: geo[2] > 0 ? geo[2] : 1100
    height: geo[3] > 0 ? geo[3] : 720
    minimumWidth: windowLayout.viewMode === "bar" ? 560 : (windowLayout.viewMode === "compact" ? 260 : 640)
    minimumHeight: windowLayout.viewMode === "bar" ? 40 : (windowLayout.viewMode === "compact" ? 120 : 480)

    Component.onCompleted: {
        taskController.setBucketFilter("active")
        if (geo[0] >= 0 && geo[1] >= 0) {
            x = geo[0]
            y = geo[1]
        }
    }

    onClosing: windowLayout.saveGeometry(windowLayout.viewMode, x, y, width, height)

    Shortcut { sequence: "Ctrl+Shift+E"; onActivated: taskController.setMenuOpen(true) }
    Shortcut { sequence: "Ctrl+Shift+1"; onActivated: windowLayout.setViewMode("expanded") }
    Shortcut { sequence: "Ctrl+Shift+2"; onActivated: windowLayout.setViewMode("bar") }
    Shortcut { sequence: "Ctrl+Shift+3"; onActivated: windowLayout.setViewMode("compact") }
    Shortcut { sequence: "Ctrl+M"; onActivated: root.spotifyOpen = !root.spotifyOpen }
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: {
        windowLayout.alwaysOnTop = !windowLayout.alwaysOnTop
        settingsController.alwaysOnTop = windowLayout.alwaysOnTop
    }}
    Shortcut { sequence: "Ctrl+Shift+B"; onActivated: backgroundController.cycleBackgroundSource() }

    Connections {
        target: timerController
        function onPhaseChanged() {
            backgroundController.phaseTint = timerController.phase
        }
    }

    BackgroundView { anchors.fill: parent; z: 0 }

    StackLayout {
        anchors.fill: parent
        z: 1
        currentIndex: windowLayout.viewMode === "compact" ? 1
                      : windowLayout.viewMode === "bar" ? 2 : 0

        // Expanded
        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                WindowChrome {
                    Layout.fillWidth: true
                    spotifyOpen: root.spotifyOpen
                    onSpotifyToggled: root.spotifyOpen = !root.spotifyOpen
                }

                ActiveTasksBar {
                    Layout.fillWidth: true
                    implicitHeight: 44
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 16

                    // Timer — centered in available space
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 20

                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 10

                                ChromeButton {
                                    text: timerController.mode === "pomodoro" ? "Pomodoro" : "Stopwatch"
                                    checked: true
                                    onClicked: timerController.setMode(
                                        timerController.mode === "pomodoro" ? "stopwatch" : "pomodoro")
                                }
                            }

                            TimerRing {
                                Layout.alignment: Qt.AlignHCenter
                                timeText: timerController.formattedTime
                                running: timerController.isRunning
                            }

                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 8

                                ChromeButton {
                                    text: "Start"
                                    accentColor: "#00d4aa"
                                    onClicked: timerController.start()
                                }
                                ChromeButton {
                                    text: "Pause"
                                    onClicked: timerController.pause()
                                }
                                ChromeButton {
                                    text: "Reset"
                                    onClicked: timerController.reset()
                                }
                                ChromeButton {
                                    text: "Skip"
                                    onClicked: timerController.skipPhase()
                                }
                            }

                            Label {
                                Layout.alignment: Qt.AlignHCenter
                                visible: !root.spotifyOpen
                                text: "Press Music or Ctrl+M to open Spotify"
                                color: "#666680"
                                font.pixelSize: 11
                            }
                        }
                    }

                    // Collapsible Spotify panel
                    SpotifyDrawer {
                        Layout.preferredWidth: root.spotifyOpen ? 340 : 0
                        Layout.fillHeight: true
                        Layout.maximumWidth: root.spotifyOpen ? 400 : 0
                        opacity: root.spotifyOpen ? 1 : 0
                        visible: width > 0 || root.spotifyOpen

                        Behavior on Layout.preferredWidth {
                            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                        }
                        Behavior on opacity {
                            NumberAnimation { duration: 180 }
                        }
                    }
                }
            }
        }

        CompactView { }

        ProgressBarView { }
    }

    TaskMenuDrawer {
        z: 10
        anchors.fill: parent
        visible: taskController.menuOpen
    }
}
