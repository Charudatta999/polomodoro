import QtQuick
import QtQuick.Layouts
import Polomodoro

// overallRatio = Σ progressMs / Σ targetMs over active tasks + descendants.
// Summing ms, not averaging ratios: a 4h task must outweigh a 5m one.
Item {
    id: root
    property bool expanded: false
    signal toggleRequested()
    implicitHeight: 20

    readonly property bool hasTargets: TaskController.overallTargetMs > 0
    readonly property real ratio: TaskController.overallRatio

    RowLayout {
        anchors.fill: parent
        spacing: Theme.sm + 1

        Rectangle {
            Layout.fillWidth: true
            height: 8
            radius: 4
            color: hover.hovered ? Theme.line : Theme.surfaceRaised
            Behavior on color { ColorAnimation { duration: Theme.dStandard } }

            Rectangle {
                width: parent.width * Math.min(1, root.ratio)
                height: parent.height
                radius: 4
                color: root.hasTargets ? Theme.accent : Theme.muted
                Behavior on width { NumberAnimation { duration: Theme.dProgress; easing.type: Easing.OutCubic } }

                // No targets: pulse. Signals "tracking, no goal" rather than
                // implying a percentage that does not exist.
                SequentialAnimation on opacity {
                    running: !root.hasTargets
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.35; duration: 1200; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 0.60; duration: 1200; easing.type: Easing.InOutSine }
                }
            }

            // The segment past target renders in overflow.
            Rectangle {
                anchors.right: parent.right
                width: parent.width * Math.min(0.35, Math.max(0, root.ratio - 1))
                height: parent.height
                radius: 4
                color: Theme.overflow
                visible: root.ratio > 1
            }
        }

        Text {
            text: root.hasTargets ? Math.round(root.ratio * 100) + "%"
                : TaskController.activeCount > 0 ? TaskController.combinedActiveLabel
                : "— no active tasks"
            font.family: Theme.monoFamily
            font.pixelSize: 11
            color: root.ratio > 1 ? Theme.overflow : Theme.textPrimary
        }

        Text {
            visible: hover.hovered || root.expanded
            text: root.expanded ? "▲" : "▼"
            font.pixelSize: 8
            color: Theme.textDim
        }
    }

    HoverHandler { id: hover }
    TapHandler {
        enabled: TaskController.activeCount > 0
        onTapped: root.toggleRequested()
    }
}
