import QtQuick
import QtQuick.Layouts
import Polomodoro

// 2-3 exclusive options with short labels; a filled pill marks the selection.
Rectangle {
    id: root
    property var options: []
    property int currentIndex: 0
    signal picked(int index)
    implicitHeight: 32
    implicitWidth: row.implicitWidth + 6
    radius: Theme.rPill
    color: Theme.bgBase

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: 3
        spacing: 2
        Repeater {
            model: root.options
            Rectangle {
                Layout.fillHeight: true
                implicitWidth: t.implicitWidth + Theme.xl
                radius: Theme.rPill
                readonly property bool on: root.currentIndex === index
                color: on ? Theme.accent : "transparent"
                Behavior on color { ColorAnimation { duration: Theme.dStandard } }
                Text {
                    id: t
                    anchors.centerIn: parent
                    text: modelData
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    font.weight: parent.on ? Font.DemiBold : Font.Normal
                    color: parent.on ? Theme.onAccent : Theme.textDim
                }
                TapHandler { onTapped: { root.currentIndex = index; root.picked(index) } }
            }
        }
    }
}
