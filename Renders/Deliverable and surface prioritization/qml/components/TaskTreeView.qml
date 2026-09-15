import QtQuick
import QtQuick.Controls
import Polomodoro

ListView {
    id: root
    required property string bucket

    clip: true
    spacing: Theme.sm + 1
    // One source of truth; the proxy only filters.
    model: TaskController.proxyFor(bucket)
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    delegate: TaskRow {
        width: root.width
        task: model
        depth: model.depth
    }

    // Each empty state names the tab's own rule, not a generic "nothing here".
    Column {
        anchors.centerIn: parent
        width: parent.width - Theme.xl
        spacing: Theme.sm
        visible: root.count === 0
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.family: Theme.fontFamily; font.pixelSize: Theme.fBodyS; font.weight: Font.Medium
            color: Theme.textPrimary
            text: root.bucket === "active" ? "Nothing active"
                : root.bucket === "future" ? "Nothing scheduled ahead"
                : "No tasks yet"
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.family: Theme.fontFamily; font.pixelSize: 10.5
            color: Theme.textDim
            text: root.bucket === "active" ? "Start a task from Pending and the clock will be credited to it."
                : root.bucket === "future" ? "Tasks with a start date in the future appear here."
                : "Create one to track time against it."
        }
    }
}
