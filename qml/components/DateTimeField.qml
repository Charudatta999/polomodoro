import QtQuick
import QtQuick.Controls
import Polomodoro

// MonthGrid in a second popup + two wrapping SpinBoxes. Stored UTC, displayed
// local — mixing the two is the likeliest source of off-by-one-day bugs here.
Rectangle {
    id: root
    property string placeholder: "Optional"
    property var utcValue: null

    implicitHeight: 38
    radius: Theme.rRow
    color: Theme.surfaceRaised
    border.width: 1
    border.color: picker.opened ? Theme.accent : Theme.line

    Row {
        anchors.verticalCenter: parent.verticalCenter
        x: Theme.md - 1
        spacing: 7
        Text { text: "▦"; font.pixelSize: 11; color: Theme.textFaint }
        Text {
            text: root.utcValue
                  ? Qt.formatDateTime(root.utcValue.toLocalTime(), "ddd d · HH:mm")
                  : root.placeholder
            font.family: Theme.monoFamily
            font.pixelSize: 12
            color: root.utcValue ? Theme.textPrimary : Theme.textFaint
        }
    }

    TapHandler { onTapped: picker.open() }

    // The month the grid is showing. MonthGrid on its own is pinned to whatever
    // month it was constructed with, so without these (and the arrows below)
    // only the current month is ever reachable.
    property int shownMonth: new Date().getMonth()
    property int shownYear: new Date().getFullYear()

    function stepMonth(delta) {
        var d = new Date(root.shownYear, root.shownMonth + delta, 1)
        root.shownMonth = d.getMonth()
        root.shownYear = d.getFullYear()
    }

    Popup {
        id: picker
        y: root.height + 4
        padding: Theme.md
        background: Rectangle { color: Theme.surface; border.color: Theme.line; border.width: 1; radius: Theme.rRow }

        // Open on the month of the current value, not on today's.
        onAboutToShow: {
            var d = root.utcValue ? root.utcValue.toLocalTime() : new Date()
            root.shownMonth = d.getMonth()
            root.shownYear = d.getFullYear()
        }

        Column {
            spacing: Theme.md

            Item {
                width: grid.width
                height: 22

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "‹"
                    font.pixelSize: 15
                    color: Theme.textDim
                    TapHandler { onTapped: root.stepMonth(-1) }
                }
                Text {
                    anchors.centerIn: parent
                    text: Qt.locale().standaloneMonthName(root.shownMonth) + " " + root.shownYear
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    color: Theme.textPrimary
                }
                Text {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: "›"
                    font.pixelSize: 15
                    color: Theme.textDim
                    TapHandler { onTapped: root.stepMonth(1) }
                }
            }

            DayOfWeekRow {
                width: grid.width
                locale: grid.locale
                delegate: Text {
                    required property string shortName
                    text: shortName
                    font.family: Theme.monoFamily
                    font.pixelSize: 10
                    color: Theme.textFaint
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            MonthGrid {
                id: grid
                locale: Qt.locale()
                month: root.shownMonth
                year: root.shownYear
                onClicked: function(date) { root.utcValue = root.combine(date, hh.value, mm.value) }
                delegate: Text {
                    required property var model
                    text: model.day
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: model.month === grid.month ? 1 : 0.28
                    color: model.today ? Theme.accent : Theme.textPrimary
                }
            }
            Row {
                spacing: Theme.sm
                SpinBox { id: hh; from: 0; to: 23; wrap: true; implicitWidth: 66 }
                Text { text: ":"; color: Theme.textFaint; anchors.verticalCenter: parent.verticalCenter }
                SpinBox { id: mm; from: 0; to: 59; stepSize: 5; wrap: true; implicitWidth: 66 }
                PillButton { label: "Clear"; small: true; onClicked: { root.utcValue = null; picker.close() } }
            }
        }
    }

    function combine(date, h, m) {
        var d = new Date(date)
        d.setHours(h, m, 0, 0)
        return d   // controller converts to UTC ISO8601 on save
    }
}
