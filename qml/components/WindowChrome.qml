import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro

RowLayout {
    id: root
    signal tasksClicked()
    signal settingsClicked()
    spacing: Theme.md

    DragHandler {
        target: null
        onActiveChanged: if (active && Window.window) Window.window.startSystemMove()
    }

    PillButton { label: "☰   Tasks"; onClicked: root.tasksClicked() }

    Text {
        text: "POLOMODORO"
        font.family: Theme.monoFamily
        font.pixelSize: 12
        font.letterSpacing: 1.7
        color: Qt.alpha(Theme.textPrimary, 0.62)
    }

    Item { Layout.fillWidth: true }

    IconButton {
        glyph: "pin"
        filled: SettingsController.alwaysOnTop
        tooltip: "Always on top (Ctrl+Shift+T)"
        onClicked: SettingsController.alwaysOnTop = !SettingsController.alwaysOnTop
    }

    BackgroundSourceToggle {}

    Rectangle {
        implicitHeight: 38
        implicitWidth: modes.implicitWidth + Theme.sm - 2
        radius: Theme.rPill
        color: Qt.rgba(0.078, 0.086, 0.106, Theme.panelOpacity)
        border.color: Theme.panelBorder
        border.width: 1

        RowLayout {
            id: modes
            anchors.centerIn: parent
            spacing: 3
            Repeater {
                model: [{ m: 0, g: "▣", t: "Expanded (Ctrl+Shift+1)" },
                        { m: 1, g: "━", t: "Bar (Ctrl+Shift+2)" },
                        { m: 2, g: "▭", t: "PiP (Ctrl+Shift+3)" }]
                Rectangle {
                    Layout.preferredHeight: 32
                    Layout.preferredWidth: 34
                    radius: Theme.rPill
                    readonly property bool on: WindowLayoutManager.viewMode === modelData.m
                    color: on ? Theme.surfaceRaised : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: modelData.g
                        font.pixelSize: 12
                        color: parent.on ? Theme.textPrimary : Qt.alpha(Theme.textPrimary, 0.6)
                    }
                    ToolTip.visible: h.hovered
                    ToolTip.text: modelData.t
                    HoverHandler { id: h }
                    TapHandler { onTapped: WindowLayoutManager.setMode(modelData.m) }
                }
            }
        }
    }

    IconButton { glyph: "settings"; tooltip: "Settings (Ctrl+,)"; onClicked: root.settingsClicked() }

    IconButton {
        glyph: "stop"
        tooltip: "Minimize"
        onClicked: if (Window.window) Window.window.visibility = Window.Minimized
    }
    IconButton {
        glyph: "close"
        tooltip: "Close"
        onClicked: Qt.quit()
    }
}
