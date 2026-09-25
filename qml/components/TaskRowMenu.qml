import QtQuick.Controls
import Polomodoro

Menu {
    id: root
    required property var task

    // Raised instead of deleting directly: a task with subtasks cascades in
    // the database, so the row owning this menu decides whether to confirm.
    signal deleteRequested()

    MenuItem { text: "Edit…"; onTriggered: TaskController.requestEdit(root.task.id) }
    MenuItem { text: "Add subtask"; onTriggered: TaskController.requestCreate(root.task.id) }
    MenuItem { text: "Mark complete"; onTriggered: TaskController.completeTask(root.task.id) }
    MenuSeparator {}
    MenuItem { text: "Promote"; enabled: root.task.depth > 0; onTriggered: TaskController.promote(root.task.id) }
    MenuItem { text: "Demote"; enabled: root.task.hasPrevSibling; onTriggered: TaskController.demote(root.task.id) }
    MenuSeparator {}
    MenuItem {
        text: "Delete"
        // Confirm only when children would cascade — that is the only case
        // where the consequence is not obvious.
        onTriggered: root.deleteRequested()
    }
}
