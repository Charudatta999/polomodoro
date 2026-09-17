import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Effects
import Polomodoro
import "windows"
import "components"

ApplicationWindow {
    id: app
    visible: true
    color: "transparent"

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
    readonly property int _sizeMax: 16777215
    property bool _moveArmed: false
    property bool _geometryTouched: false

    // Wayland startSystemMove must use the current pointer serial, but calling
    // it on every motion (a 48px bar is all "edge") makes Hyprland retile the
    // surface to a full slot. One call per press is enough.
    function beginSystemMove() {
        if (_moveArmed)
            return
        _moveArmed = true
        startSystemMove()
    }
    function resetSystemMove() { _moveArmed = false }

    function barHeightCap() {
        return SettingsController.barDropdownExpanded ? 240 : 48
    }

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
        _geometryTouched = false
        const mode = WindowLayoutManager.viewMode
        var min = _minSizeForMode[mode] || _minSizeForMode[0]
        // Lift caps before growing out of bar/PiP; Qt clamps height against
        // maximumHeight the same way it clamps against minimumHeight.
        maximumWidth = _sizeMax
        maximumHeight = _sizeMax
        minimumWidth = min.w
        minimumHeight = min.h
        x = WindowLayoutManager.x
        y = WindowLayoutManager.y
        width = WindowLayoutManager.width
        height = WindowLayoutManager.height
        if (mode === 1) {
            maximumHeight = barHeightCap()
            height = Math.min(height, maximumHeight)
        } else if (mode === 2) {
            maximumWidth = 500
            maximumHeight = 400
        }
        geometrySettleTimer.restart()
    }

    // The compositor acks asynchronously, so re-assert once before reopening
    // the persist guard and letting real user resizes through again.
    Timer {
        id: geometrySettleTimer
        interval: 250
        onTriggered: {
            if (app._geometryTouched) {
                app._applyingGeometry = false
                return
            }
            app.x = WindowLayoutManager.x
            app.y = WindowLayoutManager.y
            app.width = WindowLayoutManager.width
            app.height = WindowLayoutManager.height
            app._applyingGeometry = false
        }
    }

    Component.onCompleted: {
        applyWindowGeometry()
        if (!BackgroundController.firstRunAsked)
            wallpaperFolder.open()
    }

    FolderDialog {
        id: wallpaperFolder
        title: "Choose a wallpaper folder"
        onAccepted: {
            BackgroundController.setUserFolder(selectedFolder)
            BackgroundController.markFirstRunAsked()
        }
        onRejected: BackgroundController.markFirstRunAsked()
    }

    Connections {
        target: PaletteDeriver
        function onPaletteChanged() {
            Theme.accent = PaletteDeriver.accent
            Theme.accentHover = PaletteDeriver.accentHover
            Theme.breakColor = PaletteDeriver.breakColor
            Theme.overflow = PaletteDeriver.overflow
            Theme.muted = PaletteDeriver.muted
        }
    }

    Connections {
        target: WindowLayoutManager
        function onViewModeChanged() { app.applyWindowGeometry() }
        function onGeometryChanged() { app.applyWindowGeometry() }
    }

    onXChanged: persistGeometry()
    onYChanged: persistGeometry()
    onWidthChanged: persistGeometry()
    onHeightChanged: {
        if (!_applyingGeometry && viewMode === 1 && height > barHeightCap() + 4) {
            height = barHeightCap()
            return
        }
        persistGeometry()
    }

    function persistGeometry() {
        if (_applyingGeometry)
            return
        _geometryTouched = true
        geometrySettleTimer.stop()
        WindowLayoutManager.rememberGeometry(x, y, width, height)
    }

    Item {
        id: shell
        anchors.fill: parent

        readonly property bool wallpaperOn: app.viewMode === 0
        readonly property bool placementMiddle: wallpaperOn && SettingsController.backgroundPlacement === "middle"
        readonly property bool placementFrame: wallpaperOn && SettingsController.backgroundPlacement === "frame"

        Rectangle {
            anchors.fill: parent
            radius: Theme.rWindow
            color: Theme.bgBase
        }

        Item {
            id: bgSource
            anchors.fill: parent
            anchors.margins: 0
            visible: false
            layer.enabled: true
            layer.smooth: true
            BackgroundView {
                anchors.fill: parent
                visible: shell.wallpaperOn
            }
            Rectangle {
                anchors.fill: parent
                visible: !shell.wallpaperOn
                color: Theme.bgBase
            }
        }

        Rectangle {
            id: roundMask
            anchors.fill: bgSource
            radius: Theme.rWindow
            color: "#ffffff"
            visible: false
            layer.enabled: true
            layer.smooth: true
        }

        MultiEffect {
            anchors.fill: bgSource
            source: bgSource
            maskEnabled: true
            maskSource: roundMask
            maskThresholdMin: 0.5
        }

        Item {
            id: frameBlurMask
            anchors.fill: parent
            visible: false
            layer.enabled: true
            Rectangle {
                anchors.fill: parent
                radius: Theme.rWindow
                color: "#ffffff"
            }
            Rectangle {
                anchors.fill: parent
                anchors.margins: Theme.windowGrip
                radius: Theme.rPanel
                color: "#000000"
            }
        }

        MultiEffect {
            visible: shell.placementMiddle
            anchors.fill: parent
            source: bgSource
            blurEnabled: true
            blur: 1.0
            blurMax: 80
            maskEnabled: true
            maskSource: frameBlurMask
            maskThresholdMin: 0.5
        }

        Rectangle {
            visible: shell.placementFrame
            anchors.fill: parent
            anchors.margins: Theme.windowGrip
            radius: Theme.rPanel
            color: Qt.rgba(0.078, 0.086, 0.106, 0.55)
        }

        StackLayoutLike {
            id: stack
            anchors.fill: parent
            anchors.margins: app.viewMode === 0 ? Theme.windowGrip : 0
            current: app.viewMode
            MainWindow { id: mainWindow }
            ProgressBarView {}
            CompactView {}
        }
    }

    WindowDragFrame {
        z: 999
    }

    WindowResizeFrame {
        z: 1000
    }

    Rectangle {
        id: drawerScrim
        anchors.fill: parent
        z: 1100
        visible: taskDrawer.opened || library.opened
        color: Qt.rgba(0.024, 0.027, 0.035, 0.55)
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (library.opened)
                    library.close()
                if (taskDrawer.opened)
                    taskDrawer.close()
            }
        }
    }

    TaskMenuDrawer {
        id: taskDrawer
        parent: app.contentItem
        z: 1200
    }
    LibraryOverlay {
        id: library
        parent: app.contentItem
        z: 1200
        height: parent.height
    }
    SettingsView { id: settings; objectName: "settingsView" }

    function toggleMusic() {
        if (library.opened)
            library.close()
        else
            library.open()
    }

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
