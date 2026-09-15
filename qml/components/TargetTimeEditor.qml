import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro

RowLayout {
    id: root
    property bool touched: false
    property int totalMs: (hoursBox.value * 60 + minutesBox.value) * 60000

    SpinBox { id: hoursBox; from: 0; to: 99; onValueModified: root.touched = true }
    Text { text: "h"; color: Theme.textDim; font.family: Theme.fontFamily }
    SpinBox { id: minutesBox; from: 0; to: 59; onValueModified: root.touched = true }
    Text { text: "m"; color: Theme.textDim; font.family: Theme.fontFamily }
}
