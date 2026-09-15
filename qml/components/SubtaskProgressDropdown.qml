import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 4

    Repeater {
        model: taskController.model
        delegate: RowLayout {
            visible: model.isActive
            Layout.fillWidth: true
            spacing: 6

            Label {
                text: "  " + model.title
                color: "#d0d0e0"
                font.pixelSize: 11
                Layout.preferredWidth: 120
                elide: Text.ElideRight
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0; to: 1
                value: model.hasTarget ? model.progressRatio : 0
                implicitHeight: 8
            }

            Label {
                text: model.hasTarget
                      ? taskController.formatDuration(model.liveElapsedMs) + " / " + taskController.formatDuration(model.targetMs)
                      : taskController.formatDuration(model.liveElapsedMs)
                color: "#a0a0b8"
                font.pixelSize: 10
            }
        }
    }
}
