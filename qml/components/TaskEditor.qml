import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro

Popup {
    id: root
    modal: true
    closePolicy: Popup.CloseOnEscape
    width: 392
    padding: Theme.lg + 2
    anchors.centerIn: Overlay.overlay

    property string taskId: ""
    property string parentId: ""
    property string title: ""
    readonly property bool editing: taskId.length > 0
    readonly property string validationError: ""

    function openFor(id, parent) {
        taskId = id || ""
        parentId = parent || ""
        title = ""
        if (editing) {
            TaskController.loadInto(root, id)
            titleField.text = root.title
        } else {
            titleField.text = ""
        }
        open()
    }

    background: Rectangle {
        color: Qt.rgba(0.078, 0.086, 0.106, 0.90)
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.rPanel
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.lg - 2

        RowLayout {
            Layout.fillWidth: true
            Text { text: root.editing ? "Edit task" : "New task"; font.family: Theme.fontFamily; font.pixelSize: 15; font.weight: Font.DemiBold; color: Theme.textPrimary }
            Item { Layout.fillWidth: true }
            IconButton { glyph: "close"; onClicked: root.close() }
        }

        FieldLabel { text: "Title" }
        PoloTextField { id: titleField; Layout.fillWidth: true; placeholder: "What are you working on?" }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sm + 1
            PillButton {
                visible: root.editing
                label: "Delete"
                destructive: true
                onClicked: { TaskController.deleteTask(root.taskId); root.close() }
            }
            Item { Layout.fillWidth: true }
            PillButton { label: "Cancel"; onClicked: root.close() }
            PillButton {
                label: root.editing ? "Save" : "Create task"
                primary: true
                enabled: titleField.text.trim().length > 0
                onClicked: {
                    if (root.editing)
                        TaskController.updateTaskTitle(root.taskId, titleField.text.trim())
                    else
                        TaskController.createTask(titleField.text.trim(), root.parentId)
                    root.close()
                }
            }
        }
    }
}
