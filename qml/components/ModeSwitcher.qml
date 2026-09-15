import QtQuick
import QtQuick.Layouts
import Polomodoro

RowLayout {
    spacing: Theme.sm + 2

    PillButton {
        Layout.preferredWidth: 120
        label: TimerController.mode === "pomodoro" ? "Stopwatch" : "Pomodoro"
        onClicked: TimerController.toggleMode()
    }
    PillButton {
        Layout.preferredWidth: 168
        label: TimerController.isRunning ? "Pause" : (TimerController.isPaused ? "Resume" : "Start")
        primary: true
        onClicked: TimerController.toggle()
    }
    IconButton {
        glyph: "reset"
        tooltip: "Reset phase (Ctrl+R)"
        onClicked: TimerController.reset()
    }
}
