import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    radius: 8
    color: "#331a1a2e"
    border.color: model.isActive ? "#7c5cff" : "#22ffffff"
    implicitHeight: column.implicitHeight + 16

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: model.title
                color: "white"
                font.pixelSize: 14
                Layout.fillWidth: true
            }
            Label {
                text: model.status
                color: "#a0a0b8"
                font.pixelSize: 11
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            visible: model.hasTarget
            from: 0; to: 1
            value: model.progressRatio
        }

        Label {
            visible: model.hasTarget
            text: taskController.formatDuration(model.liveElapsedMs) + " / " + taskController.formatDuration(model.targetMs)
            color: "#a0a0b8"
            font.pixelSize: 11
        }

        RowLayout {
            spacing: 4
            ChromeButton {
                visible: model.status === "idle" || model.status === "stopped"
                text: "Start"
                implicitHeight: 28
                onClicked: taskController.startTask(model.taskId)
            }
            ChromeButton {
                visible: model.status === "active"
                text: "Pause"
                implicitHeight: 28
                onClicked: taskController.pauseTask(model.taskId)
            }
            ChromeButton {
                visible: model.status === "paused"
                text: "Resume"
                implicitHeight: 28
                onClicked: taskController.resumeTask(model.taskId)
            }
            ChromeButton {
                visible: model.status === "active" || model.status === "paused"
                text: "Stop"
                implicitHeight: 28
                onClicked: taskController.stopTask(model.taskId)
            }
            ChromeButton {
                text: "Done"
                implicitHeight: 28
                onClicked: taskController.completeTask(model.taskId)
            }
        }
    }
}
