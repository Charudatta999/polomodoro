import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import Polomodoro

// Real day timeline backed by DayTimelineModel. "planned" blocks come from a
// task's scheduled window and can be dragged to reschedule; "logged" blocks
// are recorded sessions and are fixed.
Rectangle {
    id: root
    color: Qt.rgba(0.078, 0.086, 0.106, Theme.panelOpacity)
    border.color: Theme.panelBorder
    border.width: 1
    radius: Theme.rPanel
    clip: true

    // Fixed 72 px per hour over a full 24 h column, per spec: fit-to-height
    // would compress a 12-minute session below its own label.
    readonly property int hourHeight: 72
    readonly property int gutter: 52
    readonly property int minBlockHeight: 18
    readonly property int labelMinHeight: 34

    function yForMinute(min) {
        return min / 60 * root.hourHeight
    }
    function minuteForY(y) {
        return Math.round(y / root.hourHeight * 60)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.lg
        spacing: Theme.md

        // Day strip. Each cell selects that day; the arrows step a day at a
        // time so you can walk past the seven shown.
        RowLayout {
            Layout.fillWidth: true
            spacing: 5

            IconButton {
                glyph: "prev"
                small: true
                tooltip: "Previous day"
                onClicked: DayTimelineModel.goToPreviousDay()
            }

            Repeater {
                model: DayTimelineModel.weekDays
                Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    radius: Theme.rRow
                    color: modelData.isSelected
                           ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                           : (dayHover.hovered ? Theme.surfaceRaised : "transparent")
                    border.width: modelData.isSelected ? 1 : 0
                    border.color: Theme.accent

                    HoverHandler { id: dayHover }
                    TapHandler { onTapped: DayTimelineModel.selectDate(modelData.date) }

                    Column {
                        anchors.centerIn: parent
                        spacing: 2
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData.weekday
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fEyebrow
                            color: modelData.isSelected ? Theme.accent : Theme.textFaint
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData.day
                            font.family: Theme.monoFamily
                            font.pixelSize: 12
                            font.weight: modelData.isSelected ? Font.DemiBold : Font.Normal
                            // Today stays marked even when another day is selected.
                            color: modelData.isSelected ? Theme.textPrimary
                                 : modelData.isToday ? Theme.accent : Theme.textDim
                        }
                    }
                }
            }

            IconButton {
                glyph: "next"
                small: true
                tooltip: "Next day"
                onClicked: DayTimelineModel.goToNextDay()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: DayTimelineModel.dayLabel
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fBodyS
                font.weight: Font.Medium
                color: Theme.textPrimary
            }
            PillButton {
                visible: !DayTimelineModel.showingToday
                label: "Today"
                small: true
                onClicked: DayTimelineModel.goToToday()
            }
        }

        Text {
            Layout.fillWidth: true
            text: DayTimelineModel.summaryLabel
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fEyebrow
            color: Theme.textDim
        }

        // A task with no start time has nowhere to sit on a time axis, so it
        // would otherwise just be missing here after you add it. Listing them
        // makes them visible and one click drops them onto the day.
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.xs
            visible: DayTimelineModel.unscheduled.length > 0

            Text {
                text: "UNSCHEDULED · CLICK TO PLACE ON THIS DAY"
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fEyebrow
                color: Theme.textFaint
            }

            Flow {
                Layout.fillWidth: true
                spacing: Theme.xs

                Repeater {
                    model: DayTimelineModel.unscheduled
                    Rectangle {
                        required property var modelData
                        height: 24
                        width: chipText.implicitWidth + Theme.md
                        radius: Theme.rPill
                        color: chipHover.hovered ? Theme.line : Theme.surfaceRaised
                        border.width: 1
                        border.color: Theme.line

                        Text {
                            id: chipText
                            anchors.centerIn: parent
                            text: modelData.title
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            color: Theme.textDim
                        }

                        HoverHandler { id: chipHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler {
                            onTapped: DayTimelineModel.scheduleTaskAt(
                                          modelData.taskId,
                                          DayTimelineModel.suggestedStartMinutes())
                        }
                    }
                }
            }
        }

        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: timeline.height
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            // The column is a full 24 h, so where it opens is what matters:
            // the now line on today, else an hour before the first block.
            function scrollToModelPosition() {
                var target = root.yForMinute(DayTimelineModel.scrollToMinutes)
                             - height / 3
                contentY = Math.max(0, Math.min(target, contentHeight - height))
            }

            Component.onCompleted: scrollToModelPosition()

            Connections {
                target: DayTimelineModel
                function onSelectedDateChanged() { flick.scrollToModelPosition() }
            }

            Item {
                id: timeline
                width: flick.width
                height: 24 * root.hourHeight

                Repeater {
                    model: 24
                    Rectangle {
                        required property int index
                        x: 0
                        y: index * root.hourHeight
                        width: timeline.width
                        height: 1
                        color: Theme.line
                        Text {
                            x: 0
                            y: -10
                            text: String(index).padStart(2, "0") + ":00"
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fEyebrow
                            color: Theme.textFaint
                        }
                    }
                }

                Repeater {
                    model: DayTimelineModel

                    Rectangle {
                        id: block
                        z: 5
                        required property int index
                        required property string taskId
                        required property string kind
                        required property int startMinutes
                        required property int durationMinutes
                        required property string label
                        required property bool overTarget
                        required property bool movable
                        required property int lane
                        required property int laneCount
                        required property bool collapsed

                        readonly property bool planned: kind === "planned"
                        readonly property bool running: kind === "running"
                        // Overlapping blocks share the width as columns.
                        readonly property real laneWidth:
                            (timeline.width - root.gutter - 8) / Math.max(1, laneCount)
                        readonly property color fillColor:
                            overTarget ? Theme.overflow
                                       : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.85)

                        // Past the 3-lane cap a block becomes a 4 px tick
                        // rather than squeezing every column into illegibility.
                        // Ticks fan out along the right edge so they stay
                        // individually visible and hoverable.
                        x: collapsed
                           ? timeline.width - 6 - (lane - laneCount) * 6
                           : root.gutter + Math.min(lane, laneCount - 1) * laneWidth
                        y: root.yForMinute(startMinutes)
                        width: collapsed ? 4 : laneWidth - (laneCount > 1 ? 4 : 0)
                        height: Math.max(root.minBlockHeight,
                                         durationMinutes / 60 * root.hourHeight)
                        radius: Theme.rTrough

                        // Planned is a hollow dashed outline (drawn below);
                        // running is hatched; only logged gets a solid fill.
                        color: (planned || running) ? "transparent" : fillColor
                        opacity: drag.active ? 0.75 : 1

                        // Dashed 1 px outline for a planned block — nothing has
                        // happened yet, so it must not read as solid time.
                        Shape {
                            anchors.fill: parent
                            visible: block.planned
                            antialiasing: true
                            ShapePath {
                                strokeColor: Theme.textDim
                                strokeWidth: 1
                                strokeStyle: ShapePath.DashLine
                                dashPattern: [4, 3]
                                fillColor: "transparent"
                                startX: 0.5; startY: 0.5
                                PathLine { x: block.width - 0.5; y: 0.5 }
                                PathLine { x: block.width - 0.5; y: block.height - 0.5 }
                                PathLine { x: 0.5; y: block.height - 0.5 }
                                PathLine { x: 0.5; y: 0.5 }
                            }
                        }

                        // Hatched, open-ended: an in-progress segment has no
                        // bottom edge because it has no end yet.
                        Canvas {
                            id: hatch
                            anchors.fill: parent
                            visible: block.running
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.strokeStyle = block.fillColor
                                ctx.lineWidth = 1
                                ctx.globalAlpha = 0.85
                                for (var i = -height; i < width; i += 7) {
                                    ctx.beginPath()
                                    ctx.moveTo(i, height)
                                    ctx.lineTo(i + height, 0)
                                    ctx.stroke()
                                }
                                ctx.globalAlpha = 1
                                ctx.beginPath()
                                ctx.moveTo(0.5, height)
                                ctx.lineTo(0.5, 0.5)
                                ctx.lineTo(width - 0.5, 0.5)
                                ctx.lineTo(width - 0.5, height)
                                ctx.stroke()
                            }
                            Connections {
                                target: block
                                function onHeightChanged() { hatch.requestPaint() }
                                function onWidthChanged() { hatch.requestPaint() }
                            }
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.margins: Theme.sm
                            anchors.right: parent.right
                            elide: Text.ElideRight
                            // Dropped on short blocks; the tooltip carries the
                            // title and duration instead.
                            visible: !block.collapsed
                                     && block.height >= root.labelMinHeight
                            text: block.label
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            color: (block.planned || block.running) ? Theme.textDim
                                 : block.overTarget ? Theme.bgBase : Theme.onAccent
                        }

                        ToolTip.visible: blockHover.hovered
                                         && (block.collapsed
                                             || block.height < root.labelMinHeight)
                        ToolTip.text: block.label

                        // Vertical drag reschedules. Logged history is not
                        // movable, so the handler is simply disabled there.
                        DragHandler {
                            id: drag
                            enabled: block.movable
                            xAxis.enabled: false
                            yAxis.enabled: true
                            onActiveChanged: {
                                if (active)
                                    return
                                var newStart = root.minuteForY(block.y)
                                if (!DayTimelineModel.moveBlock(block.index, newStart, 5)) {
                                    // Rejected — snap back to where the model says.
                                    block.y = Qt.binding(function() {
                                        return root.yForMinute(block.startMinutes)
                                    })
                                }
                            }
                        }
                        HoverHandler {
                            id: blockHover
                            cursorShape: block.movable ? Qt.OpenHandCursor : Qt.ArrowCursor
                        }

                        // Placing a task on the day must be reversible.
                        IconButton {
                            visible: block.movable && blockHover.hovered
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 2
                            glyph: "close"
                            small: true
                            tooltip: "Remove from this day"
                            onClicked: DayTimelineModel.unscheduleTask(block.taskId)
                        }
                    }
                }

                // NOW marker, only meaningful on today. Above the blocks (which
                // sit at z 5) so it stays readable across a filled slot.
                Rectangle {
                    z: 6
                    visible: DayTimelineModel.showingToday
                    x: root.gutter - 4
                    y: root.yForMinute(DayTimelineModel.nowMinutes)
                    width: timeline.width - root.gutter
                    height: 1
                    color: Theme.accent
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        y: -3.5
                        color: Theme.accent
                    }
                    Text {
                        anchors.right: parent.right
                        y: -14
                        text: "NOW"
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fEyebrow
                        color: Theme.accent
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: DayTimelineModel.count === 0
                    horizontalAlignment: Text.AlignHCenter
                    text: "Nothing scheduled or logged\non this day"
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    color: Theme.textFaint
                }
            }
        }
    }
}
