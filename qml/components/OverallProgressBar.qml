import QtQuick
import QtQuick.Controls

Item {
    id: root
    property real ratio: 0
    signal barClicked()
    implicitHeight: 24

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.barClicked()
    }

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: "#33ffffff"
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width * Math.min(1, Math.max(0, ratio))
        radius: 6
        color: "#7c5cff"

        Behavior on width { NumberAnimation { duration: 300 } }
    }

    Label {
        anchors.centerIn: parent
        text: ratio > 0 ? Math.round(ratio * 100) + "%" : "—"
        color: "white"
        font.pixelSize: 11
    }
}
