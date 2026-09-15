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

    Popup {
        id: picker
        y: root.height + 4
        padding: Theme.md
        background: Rectangle { color: Theme.surface; border.color: Theme.line; border.width: 1; radius: Theme.rRow }

        Column {
            spacing: Theme.md
            MonthGrid {
                id: grid
                locale: Qt.locale()
                onClicked: function(date) { root.utcValue = root.combine(date, hh.value, mm.value) }
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
