import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    property bool dropdownOpen: windowLayout.barDropdownExpanded

    Rectangle {
        id: bar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: dropdownOpen ? barColumn.implicitHeight : 48
        radius: 12
        color: "#ee12121f"
        border.color: "#33ffffff"

        Behavior on height { NumberAnimation { duration: 200 } }

        ColumnLayout {
            id: barColumn
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: timerController.formattedTime
                    font.family: "monospace"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    color: "#f0f0f5"
                    Layout.preferredWidth: 56
                }

                SpotifyMediaControls { }

                OverallProgressBar {
                    Layout.fillWidth: true
                    ratio: taskController.overallProgressRatio
                    onBarClicked: {
                        dropdownOpen = !dropdownOpen
                        windowLayout.barDropdownExpanded = dropdownOpen
                    }
                }

                Label {
                    text: spotifyBridge.nowPlayingLabel
                    color: "#b0b0c8"
                    elide: Text.ElideRight
                    font.pixelSize: 11
                    Layout.maximumWidth: 200
                    Layout.fillWidth: true
                }

                ChromeButton {
                    text: "Full"
                    implicitHeight: 28
                    onClicked: windowLayout.setViewMode("expanded")
                }
                ChromeButton {
                    text: "Tasks"
                    implicitHeight: 28
                    onClicked: taskController.setMenuOpen(true)
                }
            }

            SubtaskProgressDropdown {
                Layout.fillWidth: true
                visible: dropdownOpen
            }
        }
    }
}
