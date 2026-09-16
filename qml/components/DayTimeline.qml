import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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

    readonly property int hourHeight: 72
    readonly property int gutter: 52
    readonly property int firstMinute: DayTimelineModel.firstHour * 60

    function yForMinute(min) {
        return (min - root.firstMinute) / 60 * root.hourHeight
    }
    function minuteForY(y) {
        return Math.round(y / root.hourHeight * 60) + root.firstMinute
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

        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: timeline.height
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Item {
                id: timeline
                width: flick.width
                height: DayTimelineModel.hourCount * root.hourHeight

                Repeater {
                    model: DayTimelineModel.hourCount
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
                            text: String(DayTimelineModel.firstHour + index).padStart(2, "0") + ":00"
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

                        readonly property bool planned: kind === "planned"
                        // Overlapping blocks share the width as columns.
                        readonly property real laneWidth:
                            (timeline.width - root.gutter - 8) / Math.max(1, laneCount)

                        x: root.gutter + lane * laneWidth
                        y: root.yForMinute(startMinutes)
                        width: laneWidth - (laneCount > 1 ? 4 : 0)
                        height: Math.max(22, durationMinutes / 60 * root.hourHeight)
                        radius: Theme.rTrough

                        color: planned ? "transparent"
                             : overTarget ? Theme.overflow
                             : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.85)
                        border.color: planned ? Theme.textDim : "transparent"
                        border.width: planned ? 1 : 0
                        opacity: drag.active ? 0.75 : 1

                        Text {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.margins: Theme.sm
                            anchors.right: parent.right
                            elide: Text.ElideRight
                            text: block.label
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            color: block.planned ? Theme.textDim
                                 : block.overTarget ? Theme.bgBase : Theme.onAccent
                        }

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
                            cursorShape: block.movable ? Qt.OpenHandCursor : Qt.ArrowCursor
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
