import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro

// CloseOnEscape only — a click outside must never discard typed input.
Popup {
    id: root
    modal: true
    closePolicy: Popup.CloseOnEscape
    width: 392
    padding: Theme.lg + 2
    anchors.centerIn: Overlay.overlay

    property string taskId: ""
    property string parentId: ""
    readonly property bool editing: taskId.length > 0

    function openFor(id, parent) {
        taskId = id || ""
        parentId = parent || ""
        if (editing) TaskController.loadInto(root, id)
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

        FieldLabel { text: "Parent" }
        PoloComboBox {
            id: parentField
            Layout.fillWidth: true
            model: TaskController.parentChoices
            placeholder: "No parent — root task"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sm + 2
            ColumnLayout {
                Layout.fillWidth: true
                FieldLabel { text: "Starts" }
                // MonthGrid popup + two wrapping SpinBoxes. Never a text mask.
                DateTimeField { id: startsField; Layout.fillWidth: true; placeholder: "Optional" }
            }
            ColumnLayout {
                Layout.fillWidth: true
                FieldLabel { text: "Due" }
                DateTimeField { id: dueField; Layout.fillWidth: true; placeholder: "Optional" }
            }
        }

        FieldLabel { text: "Target duration" }
        TargetTimeEditor { id: targetField; Layout.fillWidth: true }

        Text {
            Layout.fillWidth: true
            visible: root.validationError.length > 0
            text: root.validationError
            wrapMode: Text.WordWrap
            font.family: Theme.fontFamily; font.pixelSize: 10.5
            color: "#E8489B"
        }

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
                enabled: titleField.text.trim().length > 0 && root.validationError.length === 0
                onClicked: {
                    // Store UTC; format only at the display boundary.
                    TaskController.save({
                        id: root.taskId,
                        title: titleField.text.trim(),
                        parentId: parentField.currentId || root.parentId,
                        scheduledStartAt: startsField.utcValue,
                        scheduledEndAt: dueField.utcValue,
                        targetMs: targetField.totalMs
                    })
                    root.close()
                }
            }
        }
    }

    readonly property string validationError:
        (dueField.utcValue && startsField.utcValue && dueField.utcValue < startsField.utcValue)
            ? "Due date is earlier than the start date."
            : (targetField.touched && targetField.totalMs === 0 ? "A target of zero is not a target — leave it empty instead." : "")
}
