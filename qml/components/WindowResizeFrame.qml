import QtQuick
import QtQuick.Window
import Polomodoro

// Frameless surfaces have no compositor border, so resize has to be requested
// with startSystemResize from an in-window hit strip. Corners are drawn on
// top of edges so diagonal resize wins at the intersections.
Item {
    id: root
    anchors.fill: parent
    readonly property int strip: 8
    readonly property int corner: 16

    function resize(edges) {
        const w = Window.window
        if (!w || !w.startSystemResize)
            return
        if (w.resetSystemMove)
            w.resetSystemMove()
        w.startSystemResize(edges)
    }

    component Edge: MouseArea {
        required property int edges
        required property int cursor
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: containsMouse ? cursor : Qt.ArrowCursor
        onPressed: root.resize(edges)
    }

    Edge {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.strip
        edges: Qt.LeftEdge
        cursor: Qt.SizeHorCursor
    }
    Edge {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.strip
        edges: Qt.RightEdge
        cursor: Qt.SizeHorCursor
    }
    Edge {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.strip
        edges: Qt.TopEdge
        cursor: Qt.SizeVerCursor
    }
    Edge {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.strip
        edges: Qt.BottomEdge
        cursor: Qt.SizeVerCursor
    }

    Edge {
        anchors.top: parent.top
        anchors.left: parent.left
        width: root.corner
        height: root.corner
        edges: Qt.LeftEdge | Qt.TopEdge
        cursor: Qt.SizeFDiagCursor
    }
    Edge {
        anchors.top: parent.top
        anchors.right: parent.right
        width: root.corner
        height: root.corner
        edges: Qt.RightEdge | Qt.TopEdge
        cursor: Qt.SizeBDiagCursor
    }
    Edge {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: root.corner
        height: root.corner
        edges: Qt.LeftEdge | Qt.BottomEdge
        cursor: Qt.SizeBDiagCursor
    }
    Edge {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: root.corner
        height: root.corner
        edges: Qt.RightEdge | Qt.BottomEdge
        cursor: Qt.SizeFDiagCursor
    }
}
