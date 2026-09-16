import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro

// Two integers combined to ms on accept. Never a free-text duration parser.
RowLayout {
    id: root
    property bool touched: false
    readonly property int totalMs: (hours.value * 3600 + minutes.value * 60) * 1000
    spacing: Theme.sm

    function setFromMs(ms) {
        const totalMinutes = Math.floor(ms / 60000)
        hours.value = Math.floor(totalMinutes / 60)
        minutes.value = totalMinutes % 60
        touched = false
    }

    SpinBox {
        id: hours
        from: 0; to: 23
        implicitWidth: 66
        onValueModified: root.touched = true
    }
    Text { text: "h"; font.family: Theme.monoFamily; font.pixelSize: 11; color: Theme.textFaint }
    SpinBox {
        id: minutes
        from: 0; to: 59; stepSize: 5
        wrap: true
        implicitWidth: 66
        onValueModified: root.touched = true
    }
    Text { text: "m"; font.family: Theme.monoFamily; font.pixelSize: 11; color: Theme.textFaint }

    Item { Layout.fillWidth: true }

    // Presets exist because typing 01:00 a fourth time in one session is
    // friction, not precision.
    Repeater {
        model: [{ l: "25m", h: 0, m: 25 }, { l: "1h", h: 1, m: 0 }, { l: "2h", h: 2, m: 0 }]
        PillButton {
            label: modelData.l
            small: true
            selected: hours.value === modelData.h && minutes.value === modelData.m
            onClicked: { hours.value = modelData.h; minutes.value = modelData.m; root.touched = true }
        }
    }
}
