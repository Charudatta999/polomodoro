import QtQuick
import QtQuick.Shapes
import QtQuick.Window
import Polomodoro

// Frosted picture-frame around expanded mode. Outer 8px stays on
// WindowResizeFrame; this ring is the move handle.
//
// A Rectangle with border.width ≈ radius paints square outer corners.
// Even-odd fill of two rounded rects keeps both inner and outer radii.
Item {
    id: root
    anchors.fill: parent
    visible: Window.window && Window.window.viewMode === 0

    readonly property int resizeStrip: 8
    readonly property int grip: Theme.windowGrip
    readonly property int inner: grip - resizeStrip
    readonly property real outerR: Theme.rWindow
    readonly property real innerR: Theme.rPanel

    function move() {
        const w = Window.window
        if (!w)
            return
        if (w.resetSystemMove)
            w.resetSystemMove()
        w.startSystemMove()
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        antialiasing: true

        ShapePath {
            fillColor: SettingsController.backgroundPlacement === "middle"
                       ? Qt.rgba(0.078, 0.086, 0.106, 0.22)
                       : Qt.rgba(0.078, 0.086, 0.106, Theme.panelOpacity)
            fillRule: ShapePath.OddEvenFill
            strokeWidth: 0
            strokeColor: "transparent"

            startX: root.outerR
            startY: 0
            PathLine { x: root.width - root.outerR; y: 0 }
            PathArc {
                x: root.width; y: root.outerR
                radiusX: root.outerR; radiusY: root.outerR
                direction: PathArc.Clockwise
            }
            PathLine { x: root.width; y: root.height - root.outerR }
            PathArc {
                x: root.width - root.outerR; y: root.height
                radiusX: root.outerR; radiusY: root.outerR
                direction: PathArc.Clockwise
            }
            PathLine { x: root.outerR; y: root.height }
            PathArc {
                x: 0; y: root.height - root.outerR
                radiusX: root.outerR; radiusY: root.outerR
                direction: PathArc.Clockwise
            }
            PathLine { x: 0; y: root.outerR }
            PathArc {
                x: root.outerR; y: 0
                radiusX: root.outerR; radiusY: root.outerR
                direction: PathArc.Clockwise
            }

            PathMove { x: root.grip + root.innerR; y: root.grip }
            PathLine { x: root.width - root.grip - root.innerR; y: root.grip }
            PathArc {
                x: root.width - root.grip; y: root.grip + root.innerR
                radiusX: root.innerR; radiusY: root.innerR
                direction: PathArc.Clockwise
            }
            PathLine { x: root.width - root.grip; y: root.height - root.grip - root.innerR }
            PathArc {
                x: root.width - root.grip - root.innerR; y: root.height - root.grip
                radiusX: root.innerR; radiusY: root.innerR
                direction: PathArc.Clockwise
            }
            PathLine { x: root.grip + root.innerR; y: root.height - root.grip }
            PathArc {
                x: root.grip; y: root.height - root.grip - root.innerR
                radiusX: root.innerR; radiusY: root.innerR
                direction: PathArc.Clockwise
            }
            PathLine { x: root.grip; y: root.grip + root.innerR }
            PathArc {
                x: root.grip + root.innerR; y: root.grip
                radiusX: root.innerR; radiusY: root.innerR
                direction: PathArc.Clockwise
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.outerR
        color: "transparent"
        border.width: 1
        border.color: Theme.panelBorder
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: root.grip
        radius: root.innerR
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(0.929, 0.937, 0.953, 0.08)
    }

    component Grip: MouseArea {
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        onPressed: root.move()
    }

    Grip {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.grip
        anchors.rightMargin: root.grip
        anchors.topMargin: root.resizeStrip
        height: root.inner
    }
    Grip {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.grip
        anchors.rightMargin: root.grip
        anchors.bottomMargin: root.resizeStrip
        height: root.inner
    }
    Grip {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.topMargin: root.grip
        anchors.bottomMargin: root.grip
        anchors.leftMargin: root.resizeStrip
        width: root.inner
    }
    Grip {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.topMargin: root.grip
        anchors.bottomMargin: root.grip
        anchors.rightMargin: root.resizeStrip
        width: root.inner
    }
}
