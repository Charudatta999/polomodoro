import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    property string taskId: ""
    spacing: 8

    TextField {
        id: titleField
        Layout.fillWidth: true
        placeholderText: "Task title"
    }

    RowLayout {
        Label { text: "Target (min):"; color: "white" }
        SpinBox {
            id: targetSpin
            from: 0; to: 600; value: 0
        }
    }

    Button {
        text: "Save"
        onClicked: {
            if (taskId.length)
                taskController.updateTaskTitle(taskId, titleField.text)
            taskController.updateTaskTargetMs(taskId, targetSpin.value * 60000)
        }
    }
}
