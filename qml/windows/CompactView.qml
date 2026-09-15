import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    Rectangle {
        anchors.fill: parent
        anchors.margins: 8
        radius: 12
        color: "#ee12121f"
        border.color: "#33ffffff"

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 10

            Label {
                text: timerController.formattedTime
                font.pixelSize: 36
                font.family: "monospace"
                font.weight: Font.DemiBold
                color: "#f0f0f5"
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                spacing: 6
                ChromeButton { text: "Start"; onClicked: timerController.start() }
                ChromeButton { text: "Pause"; onClicked: timerController.pause() }
                ChromeButton { text: "Reset"; onClicked: timerController.reset() }
                ChromeButton {
                    text: "Expand"
                    onClicked: windowLayout.setViewMode("expanded")
                }
            }

            Label {
                text: taskController.activeTaskCount + " active tasks"
                color: "#8888a0"
                font.pixelSize: 11
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
