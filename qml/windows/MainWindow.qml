import QtQuick
import QtQuick.Layouts
import Polomodoro
import "../components"

Item {
    id: root
    objectName: "mainWindow"

    function toggleTasks() { Window.window.toggleTaskDrawer() }
    function toggleMusic() { spotify.expanded = !spotify.expanded }
    function newTask() { Window.window.newTask() }
    function openSettings() { Window.window.openSettings() }

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

            SpotifyDrawer {
                id: spotify
            }
        }
    }

}
