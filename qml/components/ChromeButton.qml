import QtQuick
import QtQuick.Controls

ToolButton {
    id: control
    property color accentColor: "#7c5cff"

    implicitHeight: 32
    implicitWidth: Math.max(32, label.implicitWidth + 20)
    padding: 8

    background: Rectangle {
        radius: 8
        color: control.down || control.checked ? "#44ffffff"
                 : control.hovered ? "#22ffffff" : "transparent"
        border.color: control.checked ? control.accentColor : "#33ffffff"
        border.width: control.checked ? 1 : 0
    }

    contentItem: Text {
        id: label
        text: control.text
        color: control.enabled ? "#f0f0f5" : "#666680"
        font.pixelSize: 12
        font.weight: control.checked ? Font.DemiBold : Font.Normal
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
