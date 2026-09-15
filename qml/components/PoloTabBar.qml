import QtQuick
import QtQuick.Layouts
import Polomodoro

// A filled pill marks the selection — a 2px accent underline survives a
// palette change far less well against a derived colour.
Rectangle {
    id: root
    property var model: []
    property int currentIndex: 0
    implicitHeight: 38
    radius: Theme.rRow
    color: Theme.bgBase

    RowLayout {
        anchors.fill: parent
        anchors.margins: 3
        spacing: 2

        Repeater {
            model: root.model
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 6
                readonly property bool on: root.currentIndex === index
                color: on ? Theme.surfaceRaised : "transparent"
                Behavior on color { ColorAnimation { duration: Theme.dStandard } }

                RowLayout {
                    anchors.centerIn: parent
                    spacing: Theme.sm - 2
                    Text {
                        text: modelData.label
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        font.weight: parent.parent.on ? Font.DemiBold : Font.Normal
                        color: parent.parent.on ? Theme.textPrimary : Theme.textDim
                    }
                    Text {
                        // Hidden at zero, except Active — there the absence is
                        // information.
                        visible: modelData.count > 0 || modelData.always
                        text: modelData.count < 0 ? "" : modelData.count
                        font.family: Theme.monoFamily
                        font.pixelSize: 11
                        color: parent.parent.on ? Theme.accent : Theme.textFaint
                    }
                }
                TapHandler { onTapped: root.currentIndex = index }
            }
        }
    }
}
