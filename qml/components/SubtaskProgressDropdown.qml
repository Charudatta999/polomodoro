import QtQuick
import QtQuick.Layouts
import Polomodoro

// Same TaskTreeModel behind a proxy filtered to the active subtree — building
// a parallel list here lets the two views drift.
Item {
    id: root
    readonly property int rowHeight: 24
    readonly property int contentHeight: list.count * (rowHeight + 7) + Theme.md * 2

    Column {
        anchors.fill: parent
        anchors.margins: Theme.md
        spacing: 7

        Repeater {
            id: list
            model: TaskController.activeSubtreeProxy

            RowLayout {
                width: root.width - Theme.md * 2
                height: root.rowHeight
                spacing: Theme.sm + 1
                opacity: 0
                // Staggered fade-in, capped at 8 rows.
                Component.onCompleted: if (index < 8) fadeIn.start(); else opacity = 1
                NumberAnimation on opacity { id: fadeIn; running: false; to: 1; duration: 140; }

                Rectangle {
                    Layout.preferredWidth: 2
                    Layout.preferredHeight: 16
                    // Depth > 3 is flattened with a … prefix, not indented
                    // further: at 520px the title column vanishes otherwise.
                    Layout.leftMargin: Math.min(model.depth, 3) * Theme.md
                    color: model.depth === 0 ? Theme.accent : Theme.line
                }
                Text {
                    Layout.fillWidth: true
                    text: (model.depth > 3 ? "… " : "") + model.title
                    font.family: Theme.fontFamily
                    font.pixelSize: model.depth === 0 ? 11.5 : 11
                    color: model.depth === 0 ? Theme.textPrimary : Theme.textDim
                    elide: Text.ElideRight
                }
                ProgressTrough {
                    Layout.preferredWidth: 110
                    ratio: model.hasTarget ? model.progressRatio : 1
                    fill: model.hasTarget ? (model.overTarget ? Theme.overflow : Theme.accent) : Theme.muted
                }
                Text {
                    Layout.preferredWidth: 96
                    horizontalAlignment: Text.AlignRight
                    text: model.progressLabel
                    font.family: Theme.monoFamily
                    font.pixelSize: 10
                    color: model.overTarget ? Theme.overflow : (model.hasTarget ? Theme.textDim : Theme.textFaint)
                }
            }
        }
    }
}
