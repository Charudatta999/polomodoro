import QtQuick
import Polomodoro

// Fixed 72px per hour — NOT fit-to-height: a 14-hour day would compress a
// 12-minute session below its own label. Absolute y from one msToY() helper;
// never a Column.
Rectangle {
    id: root
    color: Qt.rgba(0.078, 0.086, 0.106, 0.85)
    border.color: Theme.panelBorder
    border.width: 1
    radius: Theme.rPanel

    readonly property int hourHeight: 72
    function msToY(ms) { return (ms / 3600000) * hourHeight }

    Column {
        anchors.fill: parent
        anchors.margins: Theme.lg + 2
        spacing: Theme.md + 2

        // Seven fixed columns. A month view is out of scope and this strip
        // must not imply one.
        Row {
            width: parent.width
            spacing: 5
            Repeater {
                model: DayTimelineModel.weekStrip
                Rectangle {
                    width: (parent.width - 5 * 5) / 6
                    height: 46
                    radius: 7
                    color: modelData.isSelected ? Theme.surfaceRaised : "transparent"
                    border.width: 1
                    border.color: modelData.isSelected ? Theme.accent : Theme.line
                    Column {
                        anchors.centerIn: parent
                        spacing: 1
                        Text { text: modelData.dow; font.family: Theme.monoFamily; font.pixelSize: 9; font.letterSpacing: 1; color: modelData.isSelected ? Theme.accent : Theme.textFaint }
                        Text { text: modelData.day; font.family: Theme.monoFamily; font.pixelSize: 15; color: modelData.isSelected ? Theme.textPrimary : Theme.textDim }
                    }
                    TapHandler { onTapped: DayTimelineModel.selectDay(modelData.date) }
                }
            }
        }

        Column {
            spacing: 4
            Text { text: DayTimelineModel.dayLabel; font.family: Theme.fontFamily; font.pixelSize: 16; font.weight: Font.DemiBold; color: Theme.textPrimary }
            Text { text: DayTimelineModel.summaryLabel; font.family: Theme.monoFamily; font.pixelSize: 11; font.letterSpacing: 1; color: Theme.textFaint }
        }

        Flickable {
            width: parent.width
            height: parent.height - y
            contentHeight: 24 * root.hourHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            // Opens 1h before the first block, or at the now line for today.
            Component.onCompleted: contentY = Math.max(0, root.msToY(DayTimelineModel.anchorMs) - root.hourHeight)

            Item {
                width: parent.width
                height: 24 * root.hourHeight

                Repeater {
                    model: 24
                    Item {
                        y: index * root.hourHeight
                        width: parent.width
                        height: root.hourHeight
                        Text {
                            text: (index < 10 ? "0" : "") + index + ":00"
                            font.family: Theme.monoFamily; font.pixelSize: 10; color: Theme.textFaint
                            width: 40; horizontalAlignment: Text.AlignRight
                        }
                        Rectangle { x: 50; width: parent.width - 50; height: 1; color: Theme.surfaceRaised }
                    }
                }

                // Planned: dashed outline, no fill — nothing has happened yet.
                Repeater {
                    model: DayTimelineModel.plannedBlocks
                    TimelineBlock {
                        x: 54 + modelData.lane * (parent.width - 62) / modelData.lanes
                        y: root.msToY(modelData.startMs)
                        width: (parent.width - 62) / modelData.lanes - 4
                        height: Math.max(18, root.msToY(modelData.durationMs))
                        planned: true
                        title: modelData.title
                        detail: modelData.detail
                    }
                }

                // Logged: solid accent, one per sessions row; contiguous
                // sessions under 2min apart are merged upstream.
                Repeater {
                    model: DayTimelineModel.loggedBlocks
                    TimelineBlock {
                        x: 54 + modelData.lane * (parent.width - 62) / modelData.lanes
                        y: root.msToY(modelData.startMs)
                        width: (parent.width - 62) / modelData.lanes - 4
                        height: Math.max(18, root.msToY(modelData.durationMs))
                        running: modelData.running
                        overflowMs: modelData.overflowMs
                        title: modelData.title
                        detail: modelData.detail
                    }
                }

                // Now line + in-progress block share ONE 60s timer with the
                // Future→Pending promotion, so they cannot disagree.
                Rectangle {
                    x: 50; width: parent.width - 50; height: 1
                    y: root.msToY(DayTimelineModel.nowMs)
                    color: Theme.accent
                    visible: DayTimelineModel.isToday
                    Rectangle { x: -4; y: -3; width: 8; height: 8; radius: 4; color: Theme.accent }
                    Text {
                        anchors.right: parent.right; anchors.bottom: parent.top; anchors.bottomMargin: 2
                        text: DayTimelineModel.nowLabel
                        font.family: Theme.monoFamily; font.pixelSize: 10; color: Theme.accent
                    }
                }
            }
        }
    }
}
