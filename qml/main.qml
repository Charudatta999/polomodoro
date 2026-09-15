import QtQuick
import QtQuick.Controls
import Polomodoro
import "windows"
import "components"

ApplicationWindow {
    id: app
    visible: true
    color: Theme.bgBase

    readonly property int viewMode: WindowLayoutManager.viewMode

    property bool _applyingGeometry: false

    minimumWidth:  viewMode === 1 ? 560 : (viewMode === 2 ? 260 : 640)
    minimumHeight: viewMode === 1 ? 40  : (viewMode === 2 ? 120 : 420)

    flags: (viewMode === 0 ? Qt.Window : Qt.Window | Qt.FramelessWindowHint)
           | (SettingsController.alwaysOnTop ? Qt.WindowStaysOnTopHint : 0)

    function applyWindowGeometry() {
        _applyingGeometry = true
        x = WindowLayoutManager.x
        y = WindowLayoutManager.y
        width = WindowLayoutManager.width
        height = WindowLayoutManager.height
        _applyingGeometry = false
    }

    Component.onCompleted: applyWindowGeometry()

    Connections {
        target: WindowLayoutManager
        function onViewModeChanged() { app.applyWindowGeometry() }
        function onGeometryChanged() { app.applyWindowGeometry() }
    }

    onXChanged: if (!_applyingGeometry) WindowLayoutManager.rememberGeometry(x, y, width, height)
    onYChanged: if (!_applyingGeometry) WindowLayoutManager.rememberGeometry(x, y, width, height)
    onWidthChanged: if (!_applyingGeometry) WindowLayoutManager.rememberGeometry(x, y, width, height)
    onHeightChanged: if (!_applyingGeometry) WindowLayoutManager.rememberGeometry(x, y, width, height)

    BackgroundView {
        anchors.fill: parent
        visible: app.viewMode === 0
    }

    StackLayoutLike {
        id: stack
        anchors.fill: parent
        current: app.viewMode
        MainWindow { id: mainWindow }
        ProgressBarView {}
        CompactView {}
    }

    TaskMenuDrawer { id: taskDrawer }
    SettingsView { id: settings }

    function toggleTaskDrawer() {
        if (taskDrawer.opened)
            taskDrawer.close()
        else
            taskDrawer.open()
    }

    Connections {
        target: TaskController
        function onMenuOpenChanged() {
            if (TaskController.menuOpen && !taskDrawer.opened)
                taskDrawer.open()
            else if (!TaskController.menuOpen && taskDrawer.opened)
                taskDrawer.close()
        }
    }

    function newTask() {
        taskDrawer.open()
        taskDrawer.createTask()
    }

    function openSettings() {
        settings.open()
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: BackgroundController.tick()
    }

    Shortcut { sequence: "Ctrl+Shift+1"; onActivated: WindowLayoutManager.setMode(0) }
    Shortcut { sequence: "Ctrl+Shift+2"; onActivated: WindowLayoutManager.setMode(1) }
    Shortcut { sequence: "Ctrl+Shift+3"; onActivated: WindowLayoutManager.setMode(2) }
    Shortcut { sequence: "Ctrl+Shift+P"; onActivated: WindowLayoutManager.togglePip() }
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: SettingsController.alwaysOnTop = !SettingsController.alwaysOnTop }
    Shortcut { sequence: "Ctrl+B"; onActivated: BackgroundController.cycleSource() }
    Shortcut { sequence: "Ctrl+M"; onActivated: mainWindow.toggleMusic() }
    Shortcut { sequence: "Ctrl+Shift+E"; onActivated: toggleTaskDrawer() }
    Shortcut { sequence: "Ctrl+N"; onActivated: newTask() }
    Shortcut { sequence: "Ctrl+,"; onActivated: openSettings() }
    Shortcut { sequence: "Ctrl+R"; onActivated: TimerController.reset() }
    Shortcut { sequence: "Ctrl+."; onActivated: TimerController.skipPhase() }
    Shortcut { sequence: "Ctrl+Tab"; onActivated: TimerController.toggleMode() }

    Shortcut {
        sequence: "Space"
        enabled: !(app.activeFocusItem instanceof TextInput)
        onActivated: TimerController.toggle()
    }
}
