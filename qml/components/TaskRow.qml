import QtQuick
import QtQuick.Layouts
import Polomodoro

// implicitHeight-driven: the progress bar and badge lines appear
// conditionally and the row must be free to grow. Never a fixed height.
Rectangle {
    id: root
    required property var task   // one row of TaskTreeModel
    property int depth: 0

    implicitHeight: body.implicitHeight + Theme.md * 2
    Layout.fillWidth: true
    radius: Theme.rRow
    color: hover.hovered ? Theme.surfaceRaised : Theme.surface
    border.width: 1
    border.color: task.overdue ? "#E8489B" : (selected ? Theme.accent : Theme.line)
    property bool selected: false
    opacity: task.status === "completed" ? 0.55 : (task.startable ? 1.0 : 0.45)
    enabled: task.startable || task.status === "active" || task.status === "paused"
    scale: press.pressed ? 0.99 : 1.0
    Behavior on scale { NumberAnimation { duration: 120 } }
    Behavior on color { ColorAnimation { duration: Theme.dStandard } }

    HoverHandler { id: hover }
    TapHandler { id: press }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: rowMenu.popup()
    }

    RowLayout {
        id: body
        anchors.fill: parent
        anchors.margins: Theme.md
        spacing: Theme.md - 1

        // Indent guide rail: 2px, 12px step per depth. Accent only at depth 0
        // while active — a deep tree must not become a stripe of colour.
        Rectangle {
            Layout.preferredWidth: 2
            Layout.fillHeight: true
            Layout.leftMargin: root.depth * Theme.md
            radius: 1
            color: root.depth === 0 && root.task.status === "active" ? Theme.accent
                 : root.task.status === "paused" ? Theme.muted
                 : root.task.overTarget ? Theme.overflow : Theme.line
            Behavior on color { ColorAnimation { duration: Theme.dAccent } }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.sm - 1

            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: root.task.title
                    font.family: Theme.fontFamily
                    font.pixelSize: root.depth === 0 ? Theme.fBody : Theme.fBodyS
                    font.strikeout: root.task.status === "completed"
                    color: root.depth === 0 ? Theme.textPrimary : Theme.textDim
                    elide: Text.ElideRight
                }
                Text {
                    // liveElapsedMs is recomputed by ONE app-wide 1s timer in
                    // the model — never a Timer per delegate.
                    text: root.task.progressLabel
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    color: root.task.overTarget ? Theme.overflow : Theme.textDim
                }
            }

            ProgressTrough {
                Layout.fillWidth: true
                visible: root.task.hasTarget
                ratio: root.task.progressRatio
                overflowRatio: root.task.overflowRatio
            }

            Text {
                visible: root.task.badgeText.length > 0
                text: root.task.badgeText
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fEyebrow
                color: root.task.overdue ? "#E8489B"
                     : root.task.overTarget ? Theme.overflow : Theme.textFaint
            }
        }

        // Hidden at rest to keep a long list quiet; faded in on hover.
        RowLayout {
            spacing: Theme.sm - 2
            opacity: hover.hovered || root.task.status === "active" ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.dStandard } }

            IconButton {
                visible: root.task.status === "active"
                glyph: "pause"
                onClicked: TaskController.pauseTask(root.task.id)
            }
            IconButton {
                visible: root.task.status === "paused"
                glyph: "play"; filled: true
                onClicked: TaskController.resumeTask(root.task.id)
            }
            PillButton {
                visible: root.task.status === "idle" || root.task.status === "stopped"
                label: root.task.startable ? "Start" : root.task.startsAtLabel
                enabled: root.task.startable
                onClicked: TaskController.startTask(root.task.id)
            }
            IconButton {
                visible: root.task.status === "active" || root.task.status === "paused"
                glyph: "stop"
                onClicked: TaskController.stopTask(root.task.id)
            }
        }
    }

    TaskRowMenu { id: rowMenu; task: root.task }
}
