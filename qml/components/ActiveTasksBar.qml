import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    radius: 10
    color: "#5512121f"
    border.color: "#22ffffff"
    implicitHeight: 44

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 10

        Label {
            text: "Active"
            color: "#8888a0"
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }

        Repeater {
            model: taskController.model
            delegate: RowLayout {
                visible: model.isActive
                spacing: 6

                Rectangle {
                    width: 6; height: 6; radius: 3
                    color: "#7c5cff"
                }

                Label {
                    text: model.title
                    color: "#f0f0f5"
                    font.pixelSize: 12
                }

                Label {
                    text: taskController.formatDuration(model.liveElapsedMs)
                    color: "#a0a0b8"
                    font.family: "monospace"
                    font.pixelSize: 11
                }

                ChromeButton {
                    text: "Pause"
                    implicitHeight: 26
                    onClicked: taskController.pauseTask(model.taskId)
                }
                ChromeButton {
                    text: "Stop"
                    implicitHeight: 26
                    onClicked: taskController.stopTask(model.taskId)
                }
            }
        }

        Label {
            visible: taskController.activeTaskCount === 0
            text: "No active tasks — open Tasks to add one"
            color: "#666680"
            font.pixelSize: 11
            Layout.fillWidth: true
        }

        Item { Layout.fillWidth: true; visible: taskController.activeTaskCount > 0 }
    }
}
