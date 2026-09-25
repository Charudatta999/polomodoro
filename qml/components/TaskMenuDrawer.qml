import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro

// Overlay, not push: the timer must not reflow mid-session.
Drawer {
    id: root
    edge: Qt.LeftEdge
    width: 400
    y: 0
    height: parent ? parent.height : 0
    interactive: true
    // Overlay.modal on a transparent frameless window punches alpha through
    // the whole UI (wallpaper shows through panels) and can stick after close.
    modal: false
    dim: false

    enter: Transition { NumberAnimation { property: "position"; to: 1; duration: Theme.dEnter; easing.type: Easing.OutQuint } }
    exit:  Transition { NumberAnimation { property: "position"; to: 0; duration: Theme.dEnter; easing.type: Easing.OutQuint } }

    background: Rectangle {
        color: Qt.rgba(0.055, 0.059, 0.075, 0.96)
        border.color: Theme.panelBorder
        border.width: 1
    }

    function createTask(parentId) { editor.openFor(null, parentId || null) }

    onOpenedChanged: TaskController.setMenuOpen(opened)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.xl
        spacing: Theme.lg

        RowLayout {
            Layout.fillWidth: true
            Text { text: "Tasks"; font.family: Theme.fontFamily; font.pixelSize: 19; font.weight: Font.DemiBold; color: Theme.textPrimary }
            Item { Layout.fillWidth: true }
            IconButton { glyph: "close"; onClicked: root.close() }
        }

        PoloTabBar {
            id: tabs
            Layout.fillWidth: true
            model: [
                { label: "Active",  count: TaskController.activeCount,  always: true },
                { label: "Pending", count: TaskController.pendingCount, always: false },
                { label: "Future",  count: TaskController.futureCount,  always: false },
                { label: "All",     count: -1,                          always: false }
            ]
        }

        PillButton {
            Layout.fillWidth: true
            label: "+  New task"
            primary: true
            onClicked: root.createTask()
        }

        // Four proxies over ONE TaskTreeModel — bucket is the filter role.
        // A StackLayout keeps each tab's scroll position across a switch.
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            TaskTreeView { bucket: "active" }
            TaskTreeView { bucket: "pending" }
            TaskTreeView { bucket: "future" }
            TaskTreeView { bucket: "all" }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }

        RowLayout {
            Layout.fillWidth: true
            Text { text: "Show completed"; font.family: Theme.fontFamily; font.pixelSize: Theme.fNumeric; color: Theme.textDim }
            Item { Layout.fillWidth: true }
            PoloToggle { checked: TaskController.showCompleted; onToggled: TaskController.showCompleted = checked }
        }
    }

    TaskEditor { id: editor }

    Connections {
        target: TaskController
        function onEditRequested(taskId) { editor.openFor(taskId, "") }
        function onCreateRequested(parentId) { editor.openFor(null, parentId) }
    }
}
