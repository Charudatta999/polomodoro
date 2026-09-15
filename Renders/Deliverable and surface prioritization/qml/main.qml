import QtQuick
import QtQuick.Controls
import Polomodoro
import "windows"
import "components"

ApplicationWindow {
    id: app
    visible: true
    color: "transparent"

    // WindowLayoutManager owns viewMode + all three geometry keys.
    readonly property int viewMode: WindowLayoutManager.viewMode // 0 expanded, 1 bar, 2 compact

    width:  WindowLayoutManager.width
    height: WindowLayoutManager.height
    x:      WindowLayoutManager.x
    y:      WindowLayoutManager.y

    minimumWidth:  viewMode === 1 ? 560 : (viewMode === 2 ? 260 : 640)
    minimumHeight: viewMode === 1 ? 40  : (viewMode === 2 ? 120 : 420)

    // Re-apply flags after a mode change: some Wayland compositors recreate
    // the surface and silently drop the stay-on-top hint.
    flags: (viewMode === 0 ? Qt.Window : Qt.Window | Qt.FramelessWindowHint)
           | (SettingsController.alwaysOnTop ? Qt.WindowStaysOnTopHint : 0)

    onXChanged: WindowLayoutManager.rememberGeometry(x, y, width, height)
    onYChanged: WindowLayoutManager.rememberGeometry(x, y, width, height)
    onWidthChanged: WindowLayoutManager.rememberGeometry(x, y, width, height)
    onHeightChanged: WindowLayoutManager.rememberGeometry(x, y, width, height)

    BackgroundView {
        anchors.fill: parent
        visible: app.viewMode === 0
    }

    // All three views stay instantiated: rebuilding the expanded view costs
    // ~120ms and the WebEngineView must never be reparented.
    StackLayoutLike {
        anchors.fill: parent
        current: app.viewMode
        MainWindow {}
        ProgressBarView {}
        CompactView {}
    }

    Shortcut { sequence: "Ctrl+Shift+1"; onActivated: WindowLayoutManager.setMode(0) }
    Shortcut { sequence: "Ctrl+Shift+2"; onActivated: WindowLayoutManager.setMode(1) }
    Shortcut { sequence: "Ctrl+Shift+3"; onActivated: WindowLayoutManager.setMode(2) }
    Shortcut { sequence: "Ctrl+Shift+P"; onActivated: WindowLayoutManager.togglePip() }
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: SettingsController.alwaysOnTop = !SettingsController.alwaysOnTop }
    Shortcut { sequence: "Ctrl+B"; onActivated: BackgroundController.cycleSource() }
    Shortcut { sequence: "Ctrl+M"; onActivated: mainWindow.toggleMusic() }
    Shortcut { sequence: "Ctrl+Shift+E"; onActivated: mainWindow.toggleTasks() }
    Shortcut { sequence: "Ctrl+N"; onActivated: mainWindow.newTask() }
    Shortcut { sequence: "Ctrl+,"; onActivated: mainWindow.openSettings() }

    // Window-scoped, and suppressed while a text field has focus, or typing a
    // task title would start the timer.
    Shortcut {
        sequence: "Space"
        enabled: !(app.activeFocusItem instanceof TextInput)
        onActivated: TimerController.toggle()
    }
}
