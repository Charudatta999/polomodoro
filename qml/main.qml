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

    // Starts closed: between window creation and the first applyWindowGeometry()
    // the window briefly reports its default/minimum size, and those transient
    // change signals would otherwise be persisted as if the user had resized.
    property bool _applyingGeometry: true

    // Minimum sizes are deliberately NOT bindings on viewMode. Qt clamps any
    // width/height assignment against whatever minimum is currently in force, so
    // the minimums must be lowered *before* the new geometry is written — and a
    // binding that reacts to the same viewModeChanged signal gives no ordering
    // guarantee against the handler that writes the geometry. They are set
    // imperatively in _applyWindowGeometryNow() instead.
    readonly property var _minSizeForMode: [
        { w: 640, h: 420 },   // 0 expanded
        { w: 560, h: 40  },   // 1 bar
        { w: 260, h: 120 }    // 2 compact / PiP
    ]

    minimumWidth: 640
    minimumHeight: 420

    // Every mode draws its own chrome (WindowChrome in expanded, the bar and
    // PiP backgrounds themselves), so none of them wants a system titlebar.
    // Because the hint is now identical across modes, switching modes no longer
    // changes `flags` at all — which matters more than it looks: changing flags
    // makes Wayland destroy and recreate the surface asynchronously, and that
    // recreation was landing after the width/height assignments and throwing
    // them away. That is what kept bar mode stuck at the expanded width.
    flags: Qt.Window | Qt.FramelessWindowHint
           | (SettingsController.alwaysOnTop ? Qt.WindowStaysOnTopHint : 0)

    function applyWindowGeometry() {
        Qt.callLater(_applyWindowGeometryNow)
    }

    function _applyWindowGeometryNow() {
        _applyingGeometry = true
        // Minimum first: Qt clamps an incoming size against whatever floor is
        // currently in force, so the outgoing mode's minimum has to be relaxed
        // before the new size is written.
        var min = _minSizeForMode[WindowLayoutManager.viewMode] || _minSizeForMode[0]
        minimumWidth = min.w
        minimumHeight = min.h
        x = WindowLayoutManager.x
        y = WindowLayoutManager.y
        width = WindowLayoutManager.width
        height = WindowLayoutManager.height
        geometrySettleTimer.restart()
    }

    // The compositor acks asynchronously, so re-assert once before reopening
    // the persist guard and letting real user resizes through again.
    Timer {
        id: geometrySettleTimer
        interval: 250
        onTriggered: {
            app.x = WindowLayoutManager.x
            app.y = WindowLayoutManager.y
            app.width = WindowLayoutManager.width
            app.height = WindowLayoutManager.height
            app._applyingGeometry = false
        }
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
        if (taskDrawer.opened) {
            taskDrawer.close()
            return
        }
        // The drawer is 400 px wide and as tall as the window. In bar mode the
        // window is 48 px high and in PiP 140, so it would open as an unusable
        // sliver — return to expanded, where it has room, and open it there.
        if (WindowLayoutManager.viewMode !== 0) {
            WindowLayoutManager.setMode(0)
            openDrawerAfterModeChange.restart()
            return
        }
        taskDrawer.open()
    }

    Timer {
        id: openDrawerAfterModeChange
        // Waits out the geometry settle so the drawer is sized against the
        // expanded window rather than the one we just left.
        interval: 300
        onTriggered: taskDrawer.open()
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
