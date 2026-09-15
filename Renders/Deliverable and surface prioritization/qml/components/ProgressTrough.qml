import QtQuick
import Polomodoro

Rectangle {
    id: root
    property real ratio: 0
    property real overflowRatio: 0
    property color fill: Theme.accent
    implicitHeight: 3
    radius: 2
    color: Theme.surfaceRaised
    clip: true

    Rectangle {
        width: parent.width * Math.min(1, root.ratio)
        height: parent.height
        radius: parent.radius
        color: root.fill
        Behavior on width { NumberAnimation { duration: Theme.dProgress; easing.type: Easing.OutCubic } }
        Behavior on color { ColorAnimation { duration: Theme.dAccent } }
    }
    Rectangle {
        anchors.right: parent.right
        width: parent.width * Math.min(0.35, root.overflowRatio)
        height: parent.height
        radius: parent.radius
        color: Theme.overflow
        visible: root.overflowRatio > 0
    }
}
